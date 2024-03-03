import CoreGraphics

extension CGSize {
  func aspectFit(within rect: CGRect) -> CGRect {
    let scale = min(rect.width / self.width, rect.height / self.height)
    let width = self.width * scale
    let height = self.height * scale
    return .init(x: rect.midX - width / 2, y: rect.midY - height / 2, width: width, height: height)
  }
}
