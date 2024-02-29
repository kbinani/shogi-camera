import UIKit

class MainViewController: UIViewController {
  private var current: UIView?

  override func viewDidLoad() {
    super.viewDidLoad()
    let startView = StartView()
    startView.delegate = self
    self.current = startView
    self.view.addSubview(startView)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    self.current?.frame = .init(origin: .zero, size: self.view.bounds.size)
  }
}

extension MainViewController: StartViewDelegate {
  func startViewDidStartGame(_ view: StartView, with analyzer: Analyzer) {
    let gameView = GameView(analyzer: analyzer)
    gameView.frame = .init(origin: .zero, size: self.view.bounds.size)
    self.current?.removeFromSuperview()
    self.view.addSubview(gameView)
    self.current = gameView
  }
}
