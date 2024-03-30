import AVFoundation
import Network
import ShogiCamera
import UIKit

protocol GameViewDelegate: AnyObject {
  func gameView(_ sender: GameView, presentViewController controller: UIViewController)
  func gameViewDidAbort(_ sender: GameView, server: sci.CsaServerWrapper?)
}

class GameView: UIView {
  weak var delegate: GameViewDelegate?

  let analyzer: Analyzer
  private var boardLayer: BoardLayer!
  private let reader: Reader?
  private var moveIndex: Int?
  private var wrongMoveLastNotified: Date?
  private var abortButton: RoundButton!
  private var debugButton: RoundButton?
  private var exportKifButton: RoundButton!
  private var resignButton: RoundButton!
  private var historyView: UITextView!
  private var boardRotated = false
  private let startDate: Date
  private var endDate: Date?
  private var previewLayer: AVCaptureVideoPreviewLayer!
  private var videoOverlay: VideoOverlay?
  private var stableBoardLayer: StableBoardLayer?
  private var pieceBookView: UIImageView?
  private var readYourTurn = false
  private var server: sci.CsaServerWrapper?
  private var boardOverlay: UILabel?

  private var wifiConnectivity: WifiConnectivity? {
    didSet {
      guard wifiConnectivity != oldValue else {
        return
      }
      if case .unavailable = wifiConnectivity, server != nil {
        reader?.playError()
        let controller = UIAlertController(title: "エラー", message: "WiFi がオフラインになりました", preferredStyle: .alert)
        controller.addAction(.init(title: "OK", style: .default))
        delegate?.gameView(self, presentViewController: controller)
        server = nil
        analyzer.stopGame()
        delegate?.gameViewDidAbort(self, server: nil)
      }
    }
  }

  private let kWrongMoveNotificationInterval: TimeInterval = 10

  init(analyzer: Analyzer, server: sci.CsaServerWrapper?, wifiConnectivity: WifiConnectivity?) {
    self.analyzer = analyzer
    self.server = server
    self.reader = .init()
    self.startDate = Date.now
    self.wifiConnectivity = wifiConnectivity
    super.init(frame: .zero)

    analyzer.delegate = self

    self.backgroundColor = Colors.background
    let boardLayer = BoardLayer()
    boardLayer.backgroundColor = UIColor.white.cgColor
    boardLayer.contentsScale = self.traitCollection.displayScale
    self.boardLayer = boardLayer
    self.layer.addSublayer(boardLayer)

    if let port = server?.port().value, case .available(let address) = wifiConnectivity {
      let boardOverlay = UILabel()
      boardOverlay.backgroundColor = .darkGray.withAlphaComponent(0.7)
      boardOverlay.isOpaque = false
      boardOverlay.text = "対局相手を待っています.\nサーバーアドレスは \(address),\nポートは \(port) です."
      boardOverlay.textColor = .white
      boardOverlay.font = .boldSystemFont(ofSize: 30)
      boardOverlay.textAlignment = .center
      boardOverlay.numberOfLines = 0
      self.boardOverlay = boardOverlay
      self.addSubview(boardOverlay)

      Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] timer in
        guard let self else {
          timer.invalidate()
          return
        }
        guard let server else {
          timer.invalidate()
          return
        }
        if server.isGameReady() {
          timer.invalidate()
          self.boardOverlay?.removeFromSuperview()
        }
      }
    }

    let abortButton = RoundButton(type: .custom)
    abortButton.setTitle("中断", for: .normal)
    abortButton.addTarget(self, action: #selector(abortButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(abortButton)
    self.abortButton = abortButton

    #if SHOGI_CAMERA_DEBUG
      let debugButton = RoundButton(type: .custom)
      debugButton.setTitle("デバッグ", for: .normal)
      debugButton.addTarget(self, action: #selector(debugButtonDidTouchUpInside(_:)), for: .touchUpInside)
      self.addSubview(debugButton)
      self.debugButton = debugButton
    #endif

    let resignButton = RoundButton(type: .custom)
    resignButton.setTitle("投了", for: .normal)
    resignButton.addTarget(self, action: #selector(resignButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(resignButton)
    self.resignButton = resignButton

    let historyView = UITextView(frame: .zero)
    historyView.isEditable = false
    historyView.backgroundColor = .white
    historyView.textColor = .black
    historyView.font = abortButton.titleLabel?.font
    self.addSubview(historyView)
    self.historyView = historyView

    let previewLayer = AVCaptureVideoPreviewLayer(session: self.analyzer.captureSession)
    previewLayer.videoGravity = .resizeAspect
    if #available(iOS 17, *) {
      previewLayer.connection?.videoRotationAngle = 90
    } else {
      previewLayer.connection?.videoOrientation = .portrait
    }
    self.layer.addSublayer(previewLayer)
    self.previewLayer = previewLayer

    let videoOverlay = VideoOverlay()
    videoOverlay.frame = boardLayer.frame
    self.layer.insertSublayer(videoOverlay, above: self.previewLayer)
    self.videoOverlay = videoOverlay

    let exportKifButton = RoundButton(type: .custom)
    exportKifButton.setTitle("kifuファイル", for: .normal)
    exportKifButton.setImage(.init(systemName: "square.and.arrow.up"), for: .normal)
    exportKifButton.imageTitlePadding = 10
    exportKifButton.tintColor = .white
    exportKifButton.addTarget(self, action: #selector(exportKifButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(exportKifButton)
    self.exportKifButton = exportKifButton

    let tap = UITapGestureRecognizer(target: self, action: #selector(tapGestureRecognizer(_:)))
    addGestureRecognizer(tap)
  }

  private var resigned: Bool = false {
    didSet {
      guard resigned != oldValue else {
        return
      }
      guard resigned else {
        return
      }
      self.endDate = Date.now
      self.resignButton.isEnabled = false
      self.abortButton.setTitle("戻る", for: .normal)
    }
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    var bounds = CGRect(origin: .zero, size: self.bounds.size)
    bounds.reduce(self.safeAreaInsets)
    let margin: CGFloat = 15
    bounds.expand(-margin, -margin)

    var header = bounds.removeFromTop(44)
    self.abortButton.frame = header.removeFromLeft(self.abortButton.intrinsicContentSize.width + 2 * margin)
    header.removeFromLeft(margin)
    if let debugButton {
      debugButton.frame = header.removeFromLeft(debugButton.intrinsicContentSize.width + 2 * margin)
    }
    self.resignButton.frame = header.removeFromRight(self.resignButton.intrinsicContentSize.width + 2 * margin)

    bounds.removeFromTop(margin)

    let boardBounds = bounds.removeFromTop(bounds.height / 2)
    self.boardLayer.frame = boardBounds
    self.stableBoardLayer?.frame = boardBounds
    self.pieceBookView?.frame = boardBounds
    self.boardOverlay?.frame = boardBounds

    var footer = bounds.removeFromBottom(44)
    exportKifButton.frame = footer.removeFromLeft(exportKifButton.intrinsicContentSize.width + 2 * margin)

    bounds.removeFromTop(margin)
    bounds.removeFromBottom(margin)

    let videoDimension = analyzer.dimension
    let scale = min(bounds.width / videoDimension.width, bounds.height / videoDimension.height)
    // 画面幅の 4/7 を上限として aspect fit する最大の幅で previewLayer を配置する.
    let previewWidth = min((bounds.width - margin) / 7 * 4, videoDimension.width * scale)
    let previewBounds = bounds.removeFromRight(previewWidth)
    bounds.removeFromRight(margin)
    self.historyView.frame = bounds
    self.previewLayer?.frame = previewBounds
    self.videoOverlay?.frame = previewBounds
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    self.boardLayer.contentsScale = self.traitCollection.displayScale
  }

  private var status: sci.Status? {
    didSet {
      if self.boardLayer?.isHidden != true {
        self.boardLayer?.status = status
      }
      if self.videoOverlay?.isHidden != true {
        self.videoOverlay?.status = status
      }
      if self.stableBoardLayer?.isHidden != true {
        self.stableBoardLayer?.status = status
      }
      guard let status, status.boardReady else {
        return
      }
      updateHistory()
      updateReader(oldValue)
      updatePieceBook()
    }
  }

  private func updateHistory() {
    guard let status else {
      return
    }
    var lines: [String] = []
    var last: sci.Square? = nil
    for i in 0..<status.game.moves.size() {
      let move = status.game.moves[i]
      if let last {
        let str = sci.StringFromMove(move, last)
        if let line = sci.Utility.CFStringFromU8String(str) {
          lines.append(line.takeRetainedValue() as String)
        } else {
          lines.append("エラー: 指し手を文字列に変換できませんでした")
        }
      } else {
        let str = sci.StringFromMove(move)
        if let line = sci.Utility.CFStringFromU8String(str) {
          lines.append(line.takeRetainedValue() as String)
        } else {
          lines.append("エラー: 指し手を文字列に変換できませんでした")
        }
      }
      last = move.to
    }
    if status.blackResign {
      lines.append("▲投了")
    } else if status.whiteResign {
      lines.append("△投了")
    }
    if status.aborted && !status.blackResign && !status.whiteResign {
      lines.append("中断")
    }
    let text = lines.enumerated().map({ "\($0.offset + 1): \($0.element)" }).joined(separator: "\n")
    if text != historyView.text {
      historyView.text = text
      historyView.scrollRangeToVisible(.init(location: text.utf8.count, length: 0))
    }
  }

  private func updateReader(_ oldValue: sci.Status?) {
    guard let status else {
      return
    }
    if let yourTurn = status.yourTurn.value, !readYourTurn {
      self.reader?.playYourTurn(yourTurn)
      readYourTurn = true
    }
    if !status.game.moves.empty() {
      if let moveIndex {
        if moveIndex + 1 < status.game.moves.size() {
          let mv = status.game.moves[moveIndex + 1]
          self.reader?.play(move: mv, first: status.game.first, last: status.game.moves[moveIndex])
          self.moveIndex = moveIndex + 1
        }
      } else {
        let mv = status.game.moves[0]
        self.reader?.play(move: mv, first: status.game.first, last: nil)
        self.moveIndex = 0
      }

      if status.wrongMove && !status.aborted {
        if self.wrongMoveLastNotified == nil || Date.now.timeIntervalSince(self.wrongMoveLastNotified!) > self.kWrongMoveNotificationInterval {
          self.wrongMoveLastNotified = Date.now
          if let mv = status.game.moves.last {
            let last = status.game.moves.dropLast().last
            self.reader?.playWrongMoveWarning(expected: mv, last: last)
          }
        }
      } else {
        self.wrongMoveLastNotified = nil
      }
    }
    if let moveIndex {
      if moveIndex + 1 == status.game.moves.size() {
        if (status.blackResign || status.whiteResign) && !self.resigned {
          self.reader?.playResign()
          self.resigned = true
        }
      }
    }
    if let oldValue, oldValue.waitingMove && !status.waitingMove {
      self.reader?.playNextMoveReady()
    }
    if let oldValue, status.aborted && !oldValue.aborted {
      self.reader?.playAborted()
    }
  }

  private func updatePieceBook() {
    guard let status, let pieceBookView else {
      return
    }
    guard !pieceBookView.isHidden else {
      return
    }
    let book = status.book
    guard book.__convertToBool() else {
      return
    }
    let png = book.pointee.toPng()
    var buffer: [UInt8] = []
    png.forEach { (ch: CChar) in
      buffer.append(UInt8.init(bitPattern: ch))
    }
    let data = Data(buffer)
    pieceBookView.image = UIImage(data: data)
  }

  @objc private func resignButtonDidTouchUpInside(_ sender: UIButton) {
    let controller = UIAlertController(title: nil, message: "投了しますか?", preferredStyle: .alert)
    controller.addAction(.init(title: "キャンセル", style: .cancel))
    controller.addAction(
      .init(
        title: "投了", style: .destructive,
        handler: { [weak self] _ in
          self?.resign()
        }))
    delegate?.gameView(self, presentViewController: controller)
  }

  private func resign() {
    self.resigned = true
    self.analyzer.resign()
  }

  @objc private func abortButtonDidTouchUpInside(_ sender: UIButton) {
    let message = self.resigned ? "元の画面に戻りますか?" : "対局を中断しますか?"
    let destructive = self.resigned ? "戻る" : "中断する"
    let controller = UIAlertController(title: nil, message: message, preferredStyle: .alert)
    controller.addAction(.init(title: "キャンセル", style: .cancel))
    controller.addAction(
      .init(
        title: destructive, style: .destructive,
        handler: { [weak self] _ in
          guard let self else {
            return
          }
          if let connection = previewLayer.connection {
            analyzer.captureSession.removeConnection(connection)
          }
          analyzer.stopGame()
          delegate?.gameViewDidAbort(self, server: server)
        }))
    delegate?.gameView(self, presentViewController: controller)
  }

  @objc private func tapGestureRecognizer(_ sender: UITapGestureRecognizer) {
    guard case .ended = sender.state else {
      return
    }
    if boardOverlay != nil {
      return
    }
    let location = sender.location(in: self)
    guard boardLayer.frame.contains(location) else {
      return
    }
    boardRotated.toggle()
    boardLayer.setAffineTransform(boardRotated ? .init(rotationAngle: .pi) : .identity)
  }

  @objc private func exportKifButtonDidTouchUpInside(_ sender: UIButton) {
    guard let status, let startDateString = dateTimeString(from: startDate), let userColor = analyzer.userColor,
      let opponentPlayer = analyzer.opponentPlayerName
    else {
      return
    }
    var lines: [String] = []
    lines.append("開始日時：" + startDateString)
    if let endDate, let endDateString = dateTimeString(from: endDate) {
      lines.append("終了日時：" + endDateString)
    }
    if let handicap = sci.KifStringFromHandicap(status.game.handicap_).value {
      let handicapStr = sci.Utility.CFStringFromU8String(handicap).takeRetainedValue() as String
      if status.game.handicapHand_ {
        lines.append("手合割：その他")
        lines.append("#" + handicapStr + "(駒渡し)")
      } else {
        lines.append("手合割：" + handicapStr)
      }
    } else {
      lines.append("手合割：その他")
      let handicapStr = sci.Utility.CFStringFromU8String(sci.StringFromHandicap(status.game.handicap_)).takeRetainedValue() as String
      if status.game.handicapHand_ {
        lines.append("#" + handicapStr + "(駒渡し)")
      } else {
        lines.append("#" + handicapStr)
      }
    }
    if userColor == sci.Color.Black {
      lines.append("先手：プレイヤー")
      lines.append("後手：" + opponentPlayer)
    } else {
      lines.append("先手：" + opponentPlayer)
      lines.append("後手：プレイヤー")
    }
    lines.append("手数----指手---------消費時間--")
    var last: sci.Square? = nil
    for i in 0..<status.game.moves.size() {
      var line = "\(i + 1) "
      let mv = status.game.moves[i]
      if let last, last == mv.to {
        line += "同"
      } else {
        guard let to = sci.Utility.CFStringFromU8String(sci.StringFromSquare(mv.to)) else {
          return
        }
        line += to.takeRetainedValue() as String
      }
      guard let piece = sci.Utility.CFStringFromU8String(sci.ShortStringFromPieceTypeAndStatus(mv.promote == 1 ? sci.Unpromote(mv.piece) : mv.piece)) else {
        return
      }
      line += piece.takeRetainedValue() as String
      if mv.promote == 1 {
        line += "成"
      }
      if let from = mv.from.value {
        line += "(\(9 - from.file.rawValue)\(from.rank.rawValue + 1))"
      } else {
        line += "打"
      }
      lines.append(line)
      last = mv.to
    }
    if status.blackResign {
      lines.append("\(status.game.moves.size() + 1) 投了")
      lines.append("まで\(status.game.moves.size())手で先手の勝ち")
    } else if status.whiteResign {
      lines.append("\(status.game.moves.size() + 1) 投了")
      lines.append("まで\(status.game.moves.size())手で後手の勝ち")
    }
    if status.aborted && !status.blackResign && !status.whiteResign {
      lines.append("\(status.game.moves.size() + 1) 中断")
    }
    let contents = lines.joined(separator: "\r\n")
    guard let utf8 = contents.data(using: .utf8) else {
      return
    }
    guard let name = kifuFileName(from: startDate) else {
      return
    }
    let url = FileManager.default.temporaryDirectory.appendingPathComponent(name)
    guard FileManager.default.createFile(atPath: url.path, contents: utf8) else {
      return
    }
    let c = UIActivityViewController(activityItems: [url], applicationActivities: nil)
    c.completionWithItemsHandler = { (activityType, completed, returnedItems, activityError) in
      do {
        try FileManager.default.removeItem(at: url)
      } catch {
        print(error)
      }
    }
    delegate?.gameView(self, presentViewController: c)
  }

  private func dateTimeString(from date: Date) -> String? {
    let calendar = NSCalendar.autoupdatingCurrent
    let components = calendar.dateComponents([.year, .month, .day, .hour, .minute, .second], from: date)
    guard let year = components.year, let month = components.month, let day = components.day, let hour = components.hour, let minute = components.minute, let second = components.second
    else {
      return nil
    }
    return String(format: "%d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second)
  }

  private func kifuFileName(from date: Date) -> String? {
    let calendar = NSCalendar.autoupdatingCurrent
    let components = calendar.dateComponents([.year, .month, .day, .hour, .minute, .second], from: date)
    guard let year = components.year, let month = components.month, let day = components.day, let hour = components.hour, let minute = components.minute, let second = components.second
    else {
      return nil
    }
    return String(
      format: "shogicamera_%d%02d%02d_%02d%02d%02d.kifu", year, month, day, hour, minute, second)
  }

  private let numDebugMode: Int = 2
  private var debugMode: Int = 0 {
    didSet {
      guard debugMode != oldValue else {
        return
      }
      switch debugMode {
      case 0:
        boardLayer?.isHidden = false
        stableBoardLayer?.isHidden = true
        pieceBookView?.isHidden = true
        boardLayer?.status = self.status
      case 1:
        boardLayer?.isHidden = true
        if let stableBoardLayer {
          stableBoardLayer.isHidden = false
        } else {
          let sbl = StableBoardLayer()
          sbl.status = status
          if let frame = boardLayer?.frame {
            sbl.frame = frame
          }
          self.layer.addSublayer(sbl)
          self.stableBoardLayer = sbl
        }
        pieceBookView?.isHidden = true
        stableBoardLayer?.status = status
      case 2:
        boardLayer?.isHidden = true
        stableBoardLayer?.isHidden = true
        if let pieceBookView {
          pieceBookView.isHidden = false
        } else {
          let pbv = UIImageView()
          pbv.contentMode = .scaleAspectFit
          if let frame = boardLayer?.frame {
            pbv.frame = frame
          }
          addSubview(pbv)
          self.pieceBookView = pbv
        }
        updatePieceBook()
      default:
        break
      }
    }
  }

  @objc private func debugButtonDidTouchUpInside(_ sender: UIButton) {
    debugMode = (debugMode + 1) % (numDebugMode + 1)
  }
}

extension GameView: AnalyzerDelegate {
  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.status = analyzer.status
  }
}

extension GameView: MainViewPage {
  func mainViewPageDidDetectWifiConnectivity(_ connectivity: WifiConnectivity?) {
    wifiConnectivity = connectivity
  }
}
