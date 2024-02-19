import ShogiCameraInput

typealias Move = sci.Move
typealias PieceType = sci.PieceType
typealias PieceStatus = sci.PieceStatus

extension sci.Contour {
  var cgPath: CGPath {
    let path = CGMutablePath()
    guard let first = self.points.first else {
      return path
    }
    path.move(to: first.cgPoint)
    self.points.dropFirst().forEach({ (p) in
      path.addLine(to: p.cgPoint)
    })
    path.closeSubpath()
    return path
  }
}

extension sci.PieceContour {
  var cgPath: CGPath {
    let path = CGMutablePath()
    guard let first = self.points.first else {
      return path
    }
    path.move(to: first.cgPoint)
    self.points.dropFirst().forEach({ (p) in
      path.addLine(to: p.cgPoint)
    })
    path.closeSubpath()
    return path
  }
}
