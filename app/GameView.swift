import ShogiCamera
import UIKit

protocol GameViewDelegate: AnyObject {
  func gameView(_ sender: GameView, presentViewController controller: UIViewController)
  func gameViewDidAbort(_ sender: GameView)
}

class GameView: UIView {
  weak var delegate: GameViewDelegate?

  private let analyzer: Analyzer
  private var boardLayer: BoardLayer!
  private let reader: Reader?
  private var moveIndex: Int?
  private var wrongMoveLastNotified: Date?
  private var abortButton: RoundButton!
  private var cameraButton: RoundButton!
  private var exportKifButton: RoundButton!
  private var resignButton: RoundButton!
  private var historyView: UITextView!
  private var boardRotated = false
  private let startDate: Date
  private var endDate: Date?

  private let kWrongMoveNotificationInterval: TimeInterval = 10

  init(analyzer: Analyzer) {
    self.analyzer = analyzer
    self.reader = .init()
    self.startDate = Date.now
    super.init(frame: .zero)

    analyzer.delegate = self

    self.backgroundColor = .darkGray
    let boardLayer = BoardLayer()
    boardLayer.backgroundColor = UIColor.white.cgColor
    boardLayer.contentsScale = self.traitCollection.displayScale
    self.boardLayer = boardLayer
    self.layer.addSublayer(boardLayer)

    let abortButton = RoundButton(type: .custom)
    abortButton.setTitle("中断", for: .normal)
    abortButton.addTarget(
      self, action: #selector(abortButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(abortButton)
    self.abortButton = abortButton

    let cameraButton = RoundButton(type: .custom)
    cameraButton.setTitle("カメラに切り替え", for: .normal)
    cameraButton.isHidden = true
    self.addSubview(cameraButton)
    self.cameraButton = cameraButton

    let resignButton = RoundButton(type: .custom)
    resignButton.setTitle("投了", for: .normal)
    resignButton.addTarget(
      self, action: #selector(resignButtonDidTouchUpInside(_:)), for: .touchUpInside)
    self.addSubview(resignButton)
    self.resignButton = resignButton

    let historyView = UITextView(frame: .zero)
    historyView.isEditable = false
    historyView.backgroundColor = .white
    historyView.textColor = .black
    historyView.font = abortButton.titleLabel?.font
    self.addSubview(historyView)
    self.historyView = historyView

    let exportKifButton = RoundButton(type: .custom)
    exportKifButton.setTitle("kifuファイル", for: .normal)
    exportKifButton.setImage(.init(systemName: "square.and.arrow.up"), for: .normal)
    exportKifButton.imageTitlePadding = 10
    exportKifButton.tintColor = .white
    exportKifButton.addTarget(
      self, action: #selector(exportKifButtonDidTouchUpInside(_:)), for: .touchUpInside)
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
    self.abortButton.frame = header.removeFromLeft(
      self.abortButton.intrinsicContentSize.width + 2 * margin)
    header.removeFromLeft(margin)
    self.cameraButton.frame = header.removeFromLeft(
      self.cameraButton.intrinsicContentSize.width + 2 * margin)
    self.resignButton.frame = header.removeFromRight(
      self.resignButton.intrinsicContentSize.width + 2 * margin)

    bounds.removeFromTop(margin)

    self.boardLayer.frame = bounds.removeFromTop(bounds.height / 2)

    var footer = bounds.removeFromBottom(44)
    exportKifButton.frame = footer.removeFromRight(
      exportKifButton.intrinsicContentSize.width + 2 * margin)

    bounds.removeFromTop(margin)
    bounds.removeFromBottom(margin)
    self.historyView.frame = bounds
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    self.boardLayer.contentsScale = self.traitCollection.displayScale
  }

  private var status: sci.Status? {
    didSet {
      self.boardLayer?.status = status
      guard let status, status.boardReady else {
        return
      }
      updateHistory()
      if !status.game.moves.empty() {
        if let moveIndex {
          if moveIndex + 1 < status.game.moves.size() {
            let mv = status.game.moves[moveIndex + 1]
            self.reader?.play(move: mv, last: status.game.moves[moveIndex])
            self.moveIndex = moveIndex + 1
          }
        } else {
          let mv = status.game.moves[0]
          self.reader?.play(move: mv, last: nil)
          self.moveIndex = 0
        }

        if status.wrongMove {
          if self.wrongMoveLastNotified == nil
            || Date.now.timeIntervalSince(self.wrongMoveLastNotified!)
              > self.kWrongMoveNotificationInterval
          {
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
    if lines.count % 2 == 0 {
      if status.blackResign {
        lines.append("▲投了")
      }
    } else {
      if status.whiteResign {
        lines.append("▽投了")
      }
    }
    let text = lines.enumerated().map({ "\($0.offset + 1): \($0.element)" }).joined(separator: "\n")
    if text != historyView.text {
      historyView.text = text
      historyView.scrollRangeToVisible(.init(location: text.utf8.count, length: 0))
    }
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
          delegate?.gameViewDidAbort(self)
        }))
    delegate?.gameView(self, presentViewController: controller)
  }

  @objc private func tapGestureRecognizer(_ sender: UITapGestureRecognizer) {
    guard case .ended = sender.state else {
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
    guard let status, let startDateString = dateTimeString(from: startDate), let endDate,
      let endDateString = dateTimeString(from: endDate), let userColor = analyzer.userColor,
      let aiLevel = analyzer.aiLevel
    else {
      return
    }
    var lines: [String] = []
    lines.append("開始日時：" + startDateString)
    lines.append("終了日時：" + endDateString)
    lines.append("手合割：平手")
    let playerAI: String
    if aiLevel == 0 {
      playerAI = "random"
    } else {
      playerAI = "sunfish3(maxDepth=\(aiLevel))"
    }
    if userColor == sci.Color.Black {
      lines.append("先手：プレイヤー")
      lines.append("後手：" + playerAI)
    } else {
      lines.append("先手：" + playerAI)
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
      guard
        let piece = sci.Utility.CFStringFromU8String(
          sci.ShortStringFromPieceTypeAndStatus(
            mv.promote == 1 ? sci.Unpromote(mv.piece) : mv.piece))
      else {
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
    if status.game.moves.size() % 2 == 0 {
      if status.blackResign {
        lines.append("\(status.game.moves.size() + 1) 投了")
        lines.append("まで\(status.game.moves.size())手で先手の勝ち")
      }
    } else {
      if status.whiteResign {
        lines.append("\(status.game.moves.size() + 1) 投了")
        lines.append("まで\(status.game.moves.size())手で後手の勝ち")
      }
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
    let components = calendar.dateComponents(
      [.year, .month, .day, .hour, .minute, .second], from: date)
    guard let year = components.year, let month = components.month, let day = components.day,
      let hour = components.hour, let minute = components.minute, let second = components.second
    else {
      return nil
    }
    return String(format: "%d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second)
  }

  private func kifuFileName(from date: Date) -> String? {
    let calendar = NSCalendar.autoupdatingCurrent
    let components = calendar.dateComponents(
      [.year, .month, .day, .hour, .minute, .second], from: date)
    guard let year = components.year, let month = components.month, let day = components.day,
      let hour = components.hour, let minute = components.minute, let second = components.second
    else {
      return nil
    }
    return String(
      format: "shogicamera_%d%02d%02d_%02d%02d%02d.kifu", year, month, day, hour, minute, second)
  }
}

extension GameView: AnalyzerDelegate {
  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.status = analyzer.status
  }
}
