import AVFoundation
import AudioToolbox
import ShogiCamera

class Reader {
  private let engine = AVAudioEngine()
  private let node = AVAudioPlayerNode()
  private var file: AVAudioFile!
  // 読み上げ中のメッセージが読み終わる時刻
  private var latest: Date?

  init?() {
    guard let voice = Bundle.main.url(forResource: "voice", withExtension: "wav") else {
      return nil
    }
    do {
      file = try AVAudioFile(forReading: voice)
      let session = AVAudioSession.sharedInstance()
      try session.setCategory(.playback, mode: .default)
      try session.setActive(true)
      engine.attach(node)
      engine.connect(node, to: engine.mainMixerNode, format: file.processingFormat)
      try engine.start()
      node.play()
    } catch {
      print(error)
      return nil
    }
  }

  deinit {
    node.stop()
    engine.stop()
  }

  struct Position {
    let square: (file: Int, rank: Int)
    let time: TimeInterval
  }

  struct Piece {
    let piece: UInt32
    let time: TimeInterval
  }

  enum Action: UInt32 {
    case Promote
    case NoPromote
    case Right
    case Left
    case Up
    case Down
    case Sideway  // 寄る
    case Nearest  // 直ぐ
    case Drop  // 打ち
  }

  struct ActionVoice {
    let action: UInt32
    let time: TimeInterval
  }

  enum Misc: UInt32 {
    case Black
    case White
    case Resign
    case Ready
    case WrongMoveWarningLeading
    case WrongMoveWarningTrailing
  }
  struct MiscVoice {
    let misc: UInt32
    let time: TimeInterval
  }

  private let positions: [Position] = [
    .init(square: (8, 0), time: 10.240000),
    .init(square: (8, 1), time: 11.178667),
    .init(square: (8, 2), time: 12.064000),
    .init(square: (8, 3), time: 12.906667),
    .init(square: (8, 4), time: 13.749334),
    .init(square: (8, 5), time: 14.613334),
    .init(square: (8, 6), time: 15.520001),
    .init(square: (8, 7), time: 16.405334),
    .init(square: (8, 8), time: 17.376001),
    .init(square: (7, 0), time: 18.272001),
    .init(square: (7, 1), time: 19.232001),
    .init(square: (7, 2), time: 20.117334),
    .init(square: (7, 3), time: 20.949334),
    .init(square: (7, 4), time: 21.792001),
    .init(square: (7, 5), time: 22.656001),
    .init(square: (7, 6), time: 23.562668),
    .init(square: (7, 7), time: 24.448001),
    .init(square: (7, 8), time: 25.418668),
    .init(square: (6, 0), time: 26.314668),
    .init(square: (6, 1), time: 27.285335),
    .init(square: (6, 2), time: 28.202668),
    .init(square: (6, 3), time: 29.045335),
    .init(square: (6, 4), time: 29.898668),
    .init(square: (6, 5), time: 30.805335),
    .init(square: (6, 6), time: 31.754668),
    .init(square: (6, 7), time: 32.682668),
    .init(square: (6, 8), time: 33.664001),
    .init(square: (5, 0), time: 34.581334),
    .init(square: (5, 1), time: 35.520001),
    .init(square: (5, 2), time: 36.362668),
    .init(square: (5, 3), time: 37.152001),
    .init(square: (5, 4), time: 37.941334),
    .init(square: (5, 5), time: 38.794667),
    .init(square: (5, 6), time: 39.680000),
    .init(square: (5, 7), time: 40.544000),
    .init(square: (5, 8), time: 41.461333),
    .init(square: (4, 0), time: 42.304000),
    .init(square: (4, 1), time: 43.242667),
    .init(square: (4, 2), time: 44.096000),
    .init(square: (4, 3), time: 44.928000),
    .init(square: (4, 4), time: 45.717333),
    .init(square: (4, 5), time: 46.560000),
    .init(square: (4, 6), time: 47.424000),
    .init(square: (4, 7), time: 48.277333),
    .init(square: (4, 8), time: 49.216000),
    .init(square: (3, 0), time: 50.112000),
    .init(square: (3, 1), time: 51.061333),
    .init(square: (3, 2), time: 51.914666),
    .init(square: (3, 3), time: 52.746666),
    .init(square: (3, 4), time: 53.567999),
    .init(square: (3, 5), time: 54.421332),
    .init(square: (3, 6), time: 55.349332),
    .init(square: (3, 7), time: 56.255999),
    .init(square: (3, 8), time: 57.226666),
    .init(square: (2, 0), time: 58.122666),
    .init(square: (2, 1), time: 59.050666),
    .init(square: (2, 2), time: 59.925333),
    .init(square: (2, 3), time: 60.768000),
    .init(square: (2, 4), time: 61.610667),
    .init(square: (2, 5), time: 62.496000),
    .init(square: (2, 6), time: 63.424000),
    .init(square: (2, 7), time: 64.330667),
    .init(square: (2, 8), time: 65.301334),
    .init(square: (1, 0), time: 66.218667),
    .init(square: (1, 1), time: 67.189334),
    .init(square: (1, 2), time: 68.106667),
    .init(square: (1, 3), time: 68.992000),
    .init(square: (1, 4), time: 69.866667),
    .init(square: (1, 5), time: 70.794667),
    .init(square: (1, 6), time: 71.765334),
    .init(square: (1, 7), time: 72.714667),
    .init(square: (1, 8), time: 73.760000),
    .init(square: (0, 0), time: 74.698667),
    .init(square: (0, 1), time: 75.712000),
    .init(square: (0, 2), time: 76.650667),
    .init(square: (0, 3), time: 77.536000),
    .init(square: (0, 4), time: 78.421333),
    .init(square: (0, 5), time: 79.338666),
    .init(square: (0, 6), time: 80.309333),
    .init(square: (0, 7), time: 81.237333),
    .init(square: (0, 8), time: 82.240000),
    .init(square: (Int.max, Int.max), time: 83.168000),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 83.168000),
    .init(piece: PieceType.Lance.rawValue, time: 83.776000),
    .init(piece: PieceType.Knight.rawValue, time: 84.565333),
    .init(piece: PieceType.Silver.rawValue, time: 85.386666),
    .init(piece: PieceType.Gold.rawValue, time: 85.962666),
    .init(piece: PieceType.Bishop.rawValue, time: 86.559999),
    .init(piece: PieceType.Rook.rawValue, time: 87.253332),
    .init(piece: PieceType.King.rawValue, time: 87.957332),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 88.650665),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 89.407998),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 90.325331),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 91.242664),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 92.106664),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 92.725331),
    .init(piece: UInt32.max, time: 93.375998),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 93.375998),
    .init(piece: PieceType.Lance.rawValue, time: 94.143998),
    .init(piece: PieceType.Knight.rawValue, time: 95.018665),
    .init(piece: PieceType.Silver.rawValue, time: 95.882665),
    .init(piece: PieceType.Gold.rawValue, time: 96.703998),
    .init(piece: PieceType.Bishop.rawValue, time: 97.557331),
    .init(piece: PieceType.Rook.rawValue, time: 98.442664),
    .init(piece: PieceType.King.rawValue, time: 99.370664),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 100.277331),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 101.034664),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 102.186664),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 103.327997),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 104.426664),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 105.269331),
    .init(piece: UInt32.max, time: 106.101331),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 106.101331),
    .init(action: Action.NoPromote.rawValue, time: 106.741331),
    .init(action: Action.Drop.rawValue, time: 107.509331),
    .init(action: Action.Right.rawValue, time: 108.213331),
    .init(action: Action.Left.rawValue, time: 108.885331),
    .init(action: Action.Up.rawValue, time: 109.663998),
    .init(action: Action.Down.rawValue, time: 110.367998),
    .init(action: Action.Sideway.rawValue, time: 110.997331),
    .init(action: Action.Nearest.rawValue, time: 111.605331),
    .init(action: UInt32.max, time: 112.287998),
  ]
  private let miscs: [MiscVoice] = [
    .init(misc: Misc.Black.rawValue, time: 0),
    .init(misc: Misc.White.rawValue, time: 0.821333),
    .init(misc: Misc.Resign.rawValue, time: 1.536000),
    .init(misc: Misc.Ready.rawValue, time: 2.549333),
    .init(misc: Misc.WrongMoveWarningLeading.rawValue, time: 5.888000),
    .init(misc: Misc.WrongMoveWarningTrailing.rawValue, time: 9.632000),
    .init(misc: UInt32.max, time: 10.240000),
  ]

  private var availableOffset: TimeInterval {
    if let latest {
      let now = Date.now
      let delta = latest.timeIntervalSince(now)
      return max(0, delta)
    } else {
      return 0
    }
  }

  func playWrongMoveWarning(expected: Move, last: Move?) {
    var offset = schedule(misc: .WrongMoveWarningLeading, offset: availableOffset)
    offset = schedule(move: expected, last: last, offset: offset)
    offset = schedule(misc: .WrongMoveWarningTrailing, offset: offset)
    updateLatestAfter(seconds: offset)
  }

  func playReady() {
    let offset = schedule(misc: .Ready, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  func playNextMoveReady() {
    // ct-keytone2
    AudioServicesPlaySystemSound(1075)
  }

  private func updateLatestAfter(seconds: TimeInterval) {
    latest = Date.now.addingTimeInterval(seconds)
  }

  @discardableResult
  private func schedule(misc: Misc, offset startOffset: TimeInterval) -> TimeInterval {
    let rate = file.fileFormat.sampleRate
    for i in 0..<miscs.count - 1 {
      if miscs[i].misc == misc.rawValue {
        let start = miscs[i].time
        let end = miscs[i + 1].time
        node.scheduleSegment(
          file, startingFrame: .init(startOffset + start * rate),
          frameCount: .init((end - start) * rate), at: nil)
        return startOffset + (end - start)
      }
    }
    return startOffset
  }

  func playResign() {
    let offset = schedule(misc: .Resign, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  @discardableResult
  private func schedule(move: Move, last: Move?, offset startOffset: TimeInterval) -> TimeInterval {
    var offset = startOffset
    let rate = file.fileFormat.sampleRate
    if let last, last.to == move.to {
      var startPiece: TimeInterval?
      var endPiece: TimeInterval?
      let piece =
        move.promote == 1 ? sci.PieceTypeFromPiece(move.piece).rawValue : move.piece
      for i in 0..<self.takes.count - 1 {
        let p = self.takes[i]
        if p.piece == sci.RemoveColorFromPiece(piece) {
          startPiece = p.time
          endPiece = self.takes[i + 1].time
          break
        }
      }
      if let startPiece, let endPiece {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startPiece * rate),
          frameCount: .init((endPiece - startPiece) * rate), at: nil)
        offset += endPiece - startPiece
      } else {
        print("駒読み上げ用ボイスが搭載されていない: ", move.piece)
      }
    } else {
      var startSquare: TimeInterval?
      var endSquare: TimeInterval?
      for i in 0..<self.positions.count - 1 {
        let p = self.positions[i]
        if p.square.file == Int(move.to.file.rawValue)
          && p.square.rank == Int(move.to.rank.rawValue)
        {
          startSquare = p.time
          endSquare = self.positions[i + 1].time
          break
        }
      }
      if let startSquare, let endSquare {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startSquare * rate),
          frameCount: .init((endSquare - startSquare) * rate), at: nil)
        offset += endSquare - startSquare
      } else {
        print("マス目読み上げ用ボイスが搭載されていない: file=", move.to.file.rawValue, "rank=", move.to.rank.rawValue)
      }

      let piece =
        move.promote == 1 ? sci.PieceTypeFromPiece(move.piece).rawValue : move.piece
      var startPiece: TimeInterval?
      var endPiece: TimeInterval?
      for i in 0..<self.pieces.count - 1 {
        let p = self.pieces[i]
        if p.piece == sci.RemoveColorFromPiece(piece) {
          startPiece = p.time
          endPiece = self.pieces[i + 1].time
          break
        }
      }
      if let startPiece, let endPiece {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startPiece * rate),
          frameCount: .init((endPiece - startPiece) * rate), at: nil)
        offset += endPiece - startPiece
      } else {
        print("駒読み上げ用ボイスが搭載されていない: ", move.piece)
      }
    }
    if move.promote != 0 {
      let action: Action = move.promote == 1 ? .Promote : .NoPromote
      var startAction: TimeInterval?
      var endAction: TimeInterval?
      for i in 0..<self.actions.count - 1 {
        let a = self.actions[i]
        if a.action == action.rawValue {
          startAction = a.time
          endAction = self.actions[i + 1].time
        }
      }
      if let startAction, let endAction {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startAction * rate),
          frameCount: .init((endAction - startAction) * rate), at: nil)
        offset += endAction - startAction
      } else {
        print("アクション用のボイスが無い")
      }
    }
    var actions: [Action] = []
    if (move.suffix & sci.SuffixType.Right.rawValue) == sci.SuffixType.Right.rawValue {
      actions.append(.Right)
    }
    if (move.suffix & sci.SuffixType.Left.rawValue) == sci.SuffixType.Left.rawValue {
      actions.append(.Left)
    }
    if (move.suffix & sci.SuffixType.Nearest.rawValue) == sci.SuffixType.Nearest.rawValue {
      actions.append(.Nearest)
    }
    if (move.suffix & sci.SuffixType.Up.rawValue) == sci.SuffixType.Up.rawValue {
      actions.append(.Up)
    }
    if (move.suffix & sci.SuffixType.Down.rawValue) == sci.SuffixType.Down.rawValue {
      actions.append(.Down)
    }
    if (move.suffix & sci.SuffixType.Sideway.rawValue) == sci.SuffixType.Sideway.rawValue {
      actions.append(.Sideway)
    }
    for action in actions {
      var startAction: TimeInterval?
      var endAction: TimeInterval?
      for i in 0..<self.actions.count - 1 {
        let a = self.actions[i]
        if a.action == action.rawValue {
          startAction = a.time
          endAction = self.actions[i + 1].time
          break
        }
      }
      if let startAction, let endAction {
        node.scheduleSegment(
          file, startingFrame: .init(offset + startAction * rate),
          frameCount: .init((endAction - startAction) * rate), at: nil)
        offset += endAction - startAction
      } else {
        print("アクション用のボイスが無い")
      }
    }
    return offset
  }

  func play(move: Move, last: Move?) {
    var offset = schedule(
      misc: move.color == .Black ? Misc.Black : Misc.White, offset: availableOffset)
    offset = schedule(move: move, last: last, offset: offset)
    updateLatestAfter(seconds: offset)
  }
}
