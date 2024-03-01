import ShogiCamera
import UIKit

class GameView: UIView {
  private let analyzer: Analyzer
  private var boardLayer: BoardLayer!
  private let reader: Reader?
  private var moveIndex: Int?
  private var resigned: Bool = false
  private var wrongMoveLastNotified: Date?
  private var backButton: UIButton!
  private var cameraButton: UIButton!

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

    let backButton = styleButton(UIButton(type: .custom))
    backButton.setTitle("戻る", for: .normal)
    self.addSubview(backButton)
    self.backButton = backButton

    let cameraButton = styleButton(UIButton(type: .custom))
    cameraButton.setTitle("カメラに切り替え", for: .normal)
    self.addSubview(cameraButton)
    self.cameraButton = cameraButton
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    var bounds = CGRect(origin: .zero, size: self.bounds.size)
    let margin: CGFloat = 15
    bounds.expand(-margin, -margin)

    var header = bounds.removeFromTop(44)
    self.backButton.frame = header.removeFromLeft(
      self.backButton.intrinsicContentSize.width + 2 * margin)
    header.removeFromLeft(margin)
    self.cameraButton.frame = header.removeFromLeft(
      self.cameraButton.intrinsicContentSize.width + 2 * margin)

    bounds.removeFromTop(margin)

    self.boardLayer.frame = bounds.removeFromTop(bounds.height / 2)
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

  private func styleButton(_ button: UIButton) -> UIButton {
    button.titleLabel?.textAlignment = .center
    button.layer.cornerRadius = 15
    button.backgroundColor = .black
    return button
  }
}

extension GameView: AnalyzerDelegate {
  func analyzerDidChangeBoardReadieness(_ analyzer: Analyzer) {
  }

  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.status = analyzer.status
  }
}
