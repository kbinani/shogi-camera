import ShogiCamera
import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
  var window: UIWindow?

  func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    Task.detached {
      sci.RunTests()
    }

    let window = UIWindow(frame: UIScreen.main.bounds)
    self.window = window
    window.rootViewController = ViewController()
    window.makeKeyAndVisible()
    return true
  }
}
