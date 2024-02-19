import opencv2

extension cv.Point {
  var cgPoint: CGPoint {
    return CGPoint(x: CGFloat(x), y: CGFloat(y))
  }
}

extension cv.Point2f {
  var cgPoint: CGPoint {
    return CGPoint(x: CGFloat(x), y: CGFloat(y))
  }
}
