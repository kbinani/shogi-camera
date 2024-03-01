import ShogiCamera
import UIKit

class GameView: UIView {
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
    self.addSubview(resignButton)
    self.resignButton = resignButton

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
}

extension GameView: AnalyzerDelegate {
  func analyzerDidChangeBoardReadieness(_ analyzer: Analyzer) {
  }

  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.status = analyzer.status
  }
}
