import CoreGraphics
import UIKit

extension CGRect {
  mutating func expand(_ dx: CGFloat, _ dy: CGFloat) {
    let width = max(0, self.width + dx * 2)
    let height = max(0, self.height + dy * 2)
    let cx = self.midX
    let cy = self.midY
    self.origin = .init(x: cx - width * 0.5, y: cy - height * 0.5)
    self.size = .init(width: width, height: height)
  }

  mutating func reduce(_ edgeInsets: UIEdgeInsets) {
    let cx = self.midX
    let cy = self.midY
    let left = min(cx, self.minX + edgeInsets.left)
    let right = max(cx, self.maxX - edgeInsets.right)
    let top = min(cy, self.minY + edgeInsets.top)
    let bottom = max(cy, self.maxY - edgeInsets.bottom)
    self.origin = .init(x: left, y: top)
    self.size = .init(width: right - left, height: bottom - top)
  }

  @discardableResult
  mutating func removeFromTop(_ amount: CGFloat) -> CGRect {
    let r = CGRect(x: self.minX, y: self.minY, width: self.width, height: min(amount, self.height))
    self.origin = .init(x: self.minX, y: self.minY + r.height)
    self.size = .init(width: self.width, height: self.height - r.height)
    return r
  }

  @discardableResult
  mutating func removeFromLeft(_ amount: CGFloat) -> CGRect {
    let r = CGRect(x: self.minX, y: self.minY, width: min(amount, self.width), height: self.height)
    self.origin = .init(x: self.minX + r.width, y: self.minY)
    self.size = .init(width: self.width - r.width, height: self.height)
    return r
  }

  @discardableResult
  mutating func removeFromRight(_ amount: CGFloat) -> CGRect {
    let remove = min(amount, self.width)
    let r = CGRect(
      x: self.minX + self.width - remove, y: self.minY, width: remove, height: self.height)
    self.size = .init(width: self.width - remove, height: self.height)
    return r
  }

  @discardableResult
  mutating func removeFromBottom(_ amount: CGFloat) -> CGRect {
    let remove = min(amount, self.height)
    let r = CGRect(
      x: self.minX, y: self.minY + self.height - remove, width: self.width, height: remove)
    self.size = .init(width: self.width, height: self.height - remove)
    return r
  }

  func layoutHorizontal(views: [UIView], margin: CGFloat) {
    guard !views.isEmpty else {
      return
    }
    let width = views.reduce(CGFloat(views.count - 1) * margin) { partialResult, view in
      partialResult + view.intrinsicContentSize.width
    }
    var x = self.midX - width / 2
    for view in views {
      let size = view.intrinsicContentSize
      view.frame = .init(x: x, y: self.midY - size.height / 2, width: size.width, height: size.height)
      x += size.width + margin
    }
  }
}
