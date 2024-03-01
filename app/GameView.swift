import ShogiCamera
import UIKit

protocol GameViewDelegate: AnyObject {
  func gameView(_ sender: GameView, presentAlertController controller: UIAlertController)
}

class GameView: UIView {
  weak var delegate: GameViewDelegate?

  private let analyzer: Analyzer
  private var boardLayer: BoardLayer!
  private let reader: Reader?
  private var moveIndex: Int?
  private var resigned: Bool = false
  private var wrongMoveLastNotified: Date?
  private var abortButton: RoundButton!
  private var cameraButton: RoundButton!
  private var exportKifButton: RoundButton!
  private var resignButton: RoundButton!
  private var historyView: UITextView!

  private let kWrongMoveNotificationInterval: TimeInterval = 10

  init(analyzer: Analyzer) {
    self.analyzer = analyzer
    self.reader = .init()
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
    self.addSubview(abortButton)
    self.abortButton = abortButton

    let cameraButton = RoundButton(type: .custom)
    cameraButton.setTitle("カメラに切り替え", for: .normal)
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
    exportKifButton.setTitle("KIF", for: .normal)
    exportKifButton.setImage(.init(systemName: "square.and.arrow.up"), for: .normal)
    exportKifButton.imageTitlePadding = 10
    exportKifButton.tintColor = .white
    self.addSubview(exportKifButton)
    self.exportKifButton = exportKifButton
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
            self.resignButton.isEnabled = false
            self.abortButton.setTitle("戻る", for: .normal)
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
    delegate?.gameView(self, presentAlertController: controller)
  }

  private func resign() {
    self.resigned = true
    self.resignButton.isEnabled = false
    self.abortButton.setTitle("戻る", for: .normal)
    self.analyzer.resign()
  }
}

extension GameView: AnalyzerDelegate {
  func analyzerDidChangeBoardReadieness(_ analyzer: Analyzer) {
  }

  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.status = analyzer.status
  }
}
