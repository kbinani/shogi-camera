import ShogiCamera
import UIKit
import opencv2

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
    ctx.translateBy(x: rect.minX, y: rect.minY)
    ctx.scaleBy(x: scale, y: scale)

    var minSim: CGFloat = 1
    var maxSim: CGFloat = 0
    for y in 0..<9 {
      for x in 0..<9 {
        let similarityAgainstStableBoard = Mirror(reflecting: Mirror(reflecting: status.similarityAgainstStableBoard).children[AnyIndex(x)].value).children[AnyIndex(y)].value as! Double
        minSim = min(minSim, similarityAgainstStableBoard)
        maxSim = max(maxSim, similarityAgainstStableBoard)
      }
    }
    for y in 0..<9 {
      for x in 0..<9 {
        let pw = image.size.width / 9
        let ph = image.size.height / 9
        let px = pw * CGFloat(x)
        let py = ph * CGFloat(y)
        let similarityAgainstStableBoard = Mirror(reflecting: Mirror(reflecting: status.similarityAgainstStableBoard).children[AnyIndex(x)].value).children[AnyIndex(y)].value as! Double

        let c = sci.ColorFromColormap(Float((similarityAgainstStableBoard - minSim) / (maxSim - minSim)))
        let color = UIColor(red: c.r, green: c.g, blue: c.b, alpha: 1)

        ctx.setFillColor(color.cgColor)
        let r = CGRect(x: px, y: py, width: pw, height: ph)
        ctx.fill([r])
      }
    }

    status.pieces.forEach { piece in
      guard let first = piece.pointee.points.first else {
        return
      }
      let wfirst = sci.PerspectiveTransform(first, status.perspectiveTransform, status.rotate, status.warpedWidth, status.warpedHeight)
      let path = CGMutablePath()
      path.move(to: wfirst.cgPoint)
      piece.pointee.points.dropFirst().forEach { p in
        let wp = sci.PerspectiveTransform(p, status.perspectiveTransform, status.rotate, status.warpedWidth, status.warpedHeight)
        path.addLine(to: wp.cgPoint)
      }
      path.closeSubpath()
      ctx.addPath(path)
      ctx.setFillColor(UIColor.blue.withAlphaComponent(0.2).cgColor)
      ctx.fillPath()
      ctx.addPath(path)
      ctx.setLineWidth(2)
      ctx.setStrokeColor(UIColor.blue.cgColor)
      ctx.strokePath()
    }
  }
}
