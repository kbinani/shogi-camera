import ShogiCameraInput
import UIKit

@main
class AppDelegate: UIResponder, UIApplicationDelegate {
  var window: UIWindow?

  func application(
    _ application: UIApplication,
    didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
  ) -> Bool {
    sci.RunTests()

    let window = UIWindow(frame: UIScreen.main.bounds)
    self.window = window
    window.rootViewController = ViewController()
    window.makeKeyAndVisible()
    return true
  }
}
