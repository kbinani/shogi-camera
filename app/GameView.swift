import UIKit

class GameView: UIView {
  private let analyzer: Analyzer
  private var boardLayer: BoardLayer!

  init(analyzer: Analyzer) {
    self.analyzer = analyzer
    super.init(frame: .zero)

    analyzer.delegate = self

    self.backgroundColor = .darkGray
    let boardLayer = BoardLayer()
    boardLayer.backgroundColor = UIColor.white.cgColor
    boardLayer.contentsScale = self.traitCollection.displayScale
    self.boardLayer = boardLayer
    self.layer.addSublayer(boardLayer)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    var bounds = CGRect(origin: .zero, size: self.bounds.size)
    let margin: CGFloat = 15
    bounds.expand(-margin, -margin)
    self.boardLayer.frame = bounds.removeFromTop(bounds.height / 2)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    self.boardLayer.contentsScale = self.traitCollection.displayScale
  }
}

extension GameView: AnalyzerDelegate {
  func analyzerDidChangeBoardReadieness(_ analyzer: Analyzer) {
  }

  func analyzerDidUpdateStatus(_ analyzer: Analyzer) {
    self.boardLayer?.status = analyzer.status
  }
}
