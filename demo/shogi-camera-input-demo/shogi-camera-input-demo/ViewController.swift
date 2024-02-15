import UIKit

class ViewController: UIViewController {
  private weak var debugView: DebugView?

  override func viewDidLoad() {
    super.viewDidLoad()
    let debugView = DebugView()
    self.view.addSubview(debugView)
    self.debugView = debugView
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    debugView?.frame = .init(origin: .zero, size: self.view.bounds.size)
  }
}
