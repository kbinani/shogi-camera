import ShogiCamera
import UIKit

class StableBoardLayer: CALayer {
  var status: sci.Status? {
    didSet {
      setNeedsDisplay()
    }
  }

  override func draw(in ctx: CGContext) {
    guard let status else {
      return
    }
    guard let board = status.stableBoard.value else {
      return
    }
    guard let ptr = sci.Utility.UIImageFromMat(board) else {
      return
    }
    let image = Unmanaged<UIImage>.fromOpaque(ptr).takeRetainedValue()
    guard let cgImage = image.cgImage else {
      return
    }
    ctx.saveGState()
    defer {
      ctx.restoreGState()
    }
    let width = self.bounds.width
    let height = self.bounds.height
    let scale = min(width / image.size.width, height / image.size.height)
    let rect = CGRect(
      x: width * 0.5 - image.size.width * 0.5 * scale,
      y: height * 0.5 - image.size.height * 0.5 * scale, width: image.size.width * scale,
      height: image.size.height * scale)
    do {
      ctx.saveGState()
      defer {
        ctx.restoreGState()
      }
      ctx.translateBy(x: 0, y: height)
      ctx.scaleBy(x: 1, y: -1)
      ctx.draw(cgImage, in: rect)
    }
    let minSim: CGFloat = 0
    let maxSim: CGFloat = status.stableBoardMaxSimilarity
    for y in 0..<9 {
      for x in 0..<9 {
        let pw = rect.width / 9
        let ph = rect.height / 9
        let px = rect.minX + pw * CGFloat(x)
        let py = rect.minY + ph * CGFloat(y)
        let similarity =
          Mirror(reflecting: Mirror(reflecting: status.similarity).children[AnyIndex(x)].value)
          .children[AnyIndex(y)].value as! Double
        let similarityAgainstStableBoard =
          Mirror(
            reflecting: Mirror(reflecting: status.similarityAgainstStableBoard).children[
              AnyIndex(x)
            ].value
          ).children[AnyIndex(y)].value as! Double
        let bar = min((similarity - minSim) / (maxSim - minSim), 1) * ph
        let sbar =
          min((similarityAgainstStableBoard - minSim) / (maxSim - minSim), 1) * ph

        let r = CGRect(x: px, y: py + ph - bar, width: pw * 0.5, height: bar)
        ctx.setFillColor(UIColor.red.withAlphaComponent(0.2).cgColor)
        ctx.fill([r])
        ctx.setStrokeColor(UIColor.red.cgColor)
        ctx.stroke(r)

        let r2 = CGRect(x: px + pw * 0.5, y: py + ph - sbar, width: pw * 0.5, height: sbar)
        ctx.setFillColor(UIColor.blue.withAlphaComponent(0.2).cgColor)
        ctx.fill([r2])
        ctx.setStrokeColor(UIColor.blue.cgColor)
        ctx.stroke(r2)
      }
    }
  }
}
