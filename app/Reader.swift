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
    case YourTurnBlack
    case YourTurnWhite
    case Aborted
    case Error
  }
  struct MiscVoice {
    let misc: UInt32
    let time: TimeInterval
  }

  private let positions: [Position] = [
    .init(square: (8, 0), time: 16.533333),
    .init(square: (8, 1), time: 17.472000),
    .init(square: (8, 2), time: 18.357333),
    .init(square: (8, 3), time: 19.200000),
    .init(square: (8, 4), time: 20.042667),
    .init(square: (8, 5), time: 20.906667),
    .init(square: (8, 6), time: 21.813334),
    .init(square: (8, 7), time: 22.698667),
    .init(square: (8, 8), time: 23.669334),
    .init(square: (7, 0), time: 24.565334),
    .init(square: (7, 1), time: 25.525334),
    .init(square: (7, 2), time: 26.410667),
    .init(square: (7, 3), time: 27.242667),
    .init(square: (7, 4), time: 28.085334),
    .init(square: (7, 5), time: 28.949334),
    .init(square: (7, 6), time: 29.856001),
    .init(square: (7, 7), time: 30.741334),
    .init(square: (7, 8), time: 31.712001),
    .init(square: (6, 0), time: 32.608001),
    .init(square: (6, 1), time: 33.578668),
    .init(square: (6, 2), time: 34.496001),
    .init(square: (6, 3), time: 35.338668),
    .init(square: (6, 4), time: 36.192001),
    .init(square: (6, 5), time: 37.098668),
    .init(square: (6, 6), time: 38.048001),
    .init(square: (6, 7), time: 38.976001),
    .init(square: (6, 8), time: 39.957334),
    .init(square: (5, 0), time: 40.874667),
    .init(square: (5, 1), time: 41.813334),
    .init(square: (5, 2), time: 42.656001),
    .init(square: (5, 3), time: 43.445334),
    .init(square: (5, 4), time: 44.234667),
    .init(square: (5, 5), time: 45.088000),
    .init(square: (5, 6), time: 45.973333),
    .init(square: (5, 7), time: 46.837333),
    .init(square: (5, 8), time: 47.754666),
    .init(square: (4, 0), time: 48.597333),
    .init(square: (4, 1), time: 49.536000),
    .init(square: (4, 2), time: 50.389333),
    .init(square: (4, 3), time: 51.221333),
    .init(square: (4, 4), time: 52.010666),
    .init(square: (4, 5), time: 52.853333),
    .init(square: (4, 6), time: 53.717333),
    .init(square: (4, 7), time: 54.570666),
    .init(square: (4, 8), time: 55.509333),
    .init(square: (3, 0), time: 56.405333),
    .init(square: (3, 1), time: 57.354666),
    .init(square: (3, 2), time: 58.207999),
    .init(square: (3, 3), time: 59.039999),
    .init(square: (3, 4), time: 59.861332),
    .init(square: (3, 5), time: 60.714665),
    .init(square: (3, 6), time: 61.642665),
    .init(square: (3, 7), time: 62.549332),
    .init(square: (3, 8), time: 63.519999),
    .init(square: (2, 0), time: 64.415999),
    .init(square: (2, 1), time: 65.343999),
    .init(square: (2, 2), time: 66.218666),
    .init(square: (2, 3), time: 67.061333),
    .init(square: (2, 4), time: 67.904000),
    .init(square: (2, 5), time: 68.789333),
    .init(square: (2, 6), time: 69.717333),
    .init(square: (2, 7), time: 70.624000),
    .init(square: (2, 8), time: 71.594667),
    .init(square: (1, 0), time: 72.512000),
    .init(square: (1, 1), time: 73.482667),
    .init(square: (1, 2), time: 74.400000),
    .init(square: (1, 3), time: 75.285333),
    .init(square: (1, 4), time: 76.160000),
    .init(square: (1, 5), time: 77.088000),
    .init(square: (1, 6), time: 78.058667),
    .init(square: (1, 7), time: 79.008000),
    .init(square: (1, 8), time: 80.053333),
    .init(square: (0, 0), time: 80.992000),
    .init(square: (0, 1), time: 82.005333),
    .init(square: (0, 2), time: 82.944000),
    .init(square: (0, 3), time: 83.829333),
    .init(square: (0, 4), time: 84.714666),
    .init(square: (0, 5), time: 85.631999),
    .init(square: (0, 6), time: 86.602666),
    .init(square: (0, 7), time: 87.530666),
    .init(square: (0, 8), time: 88.533333),
    .init(square: (Int.max, Int.max), time: 89.461333),
  ]
  private let pieces: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 89.461333),
    .init(piece: PieceType.Lance.rawValue, time: 90.069333),
    .init(piece: PieceType.Knight.rawValue, time: 90.858666),
    .init(piece: PieceType.Silver.rawValue, time: 91.679999),
    .init(piece: PieceType.Gold.rawValue, time: 92.255999),
    .init(piece: PieceType.Bishop.rawValue, time: 92.853332),
    .init(piece: PieceType.Rook.rawValue, time: 93.546665),
    .init(piece: PieceType.King.rawValue, time: 94.250665),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 94.943998),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 95.701331),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 96.618664),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 97.535997),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 98.399997),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 99.018664),
    .init(piece: UInt32.max, time: 99.669331),
  ]
  private let takes: [Piece] = [
    .init(piece: PieceType.Pawn.rawValue, time: 99.669331),
    .init(piece: PieceType.Lance.rawValue, time: 100.437331),
    .init(piece: PieceType.Knight.rawValue, time: 101.311998),
    .init(piece: PieceType.Silver.rawValue, time: 102.175998),
    .init(piece: PieceType.Gold.rawValue, time: 102.997331),
    .init(piece: PieceType.Bishop.rawValue, time: 103.850664),
    .init(piece: PieceType.Rook.rawValue, time: 104.735997),
    .init(piece: PieceType.King.rawValue, time: 105.663997),
    .init(piece: PieceType.Pawn.rawValue + PieceStatus.Promoted.rawValue, time: 106.570664),
    .init(piece: PieceType.Lance.rawValue + PieceStatus.Promoted.rawValue, time: 107.327997),
    .init(piece: PieceType.Knight.rawValue + PieceStatus.Promoted.rawValue, time: 108.479997),
    .init(piece: PieceType.Silver.rawValue + PieceStatus.Promoted.rawValue, time: 109.621330),
    .init(piece: PieceType.Bishop.rawValue + PieceStatus.Promoted.rawValue, time: 110.719997),
    .init(piece: PieceType.Rook.rawValue + PieceStatus.Promoted.rawValue, time: 111.562664),
    .init(piece: UInt32.max, time: 112.394664),
  ]
  private let actions: [ActionVoice] = [
    .init(action: Action.Promote.rawValue, time: 112.394664),
    .init(action: Action.NoPromote.rawValue, time: 113.034664),
    .init(action: Action.Drop.rawValue, time: 113.802664),
    .init(action: Action.Right.rawValue, time: 114.506664),
    .init(action: Action.Left.rawValue, time: 115.178664),
    .init(action: Action.Up.rawValue, time: 115.957331),
    .init(action: Action.Down.rawValue, time: 116.661331),
    .init(action: Action.Sideway.rawValue, time: 117.290664),
    .init(action: Action.Nearest.rawValue, time: 117.898664),
    .init(action: UInt32.max, time: 118.581331),
  ]
  private let miscs: [MiscVoice] = [
    .init(misc: Misc.Black.rawValue, time: 0),
    .init(misc: Misc.White.rawValue, time: 0.821333),
    .init(misc: Misc.Resign.rawValue, time: 1.536000),
    .init(misc: Misc.Ready.rawValue, time: 2.549333),
    .init(misc: Misc.WrongMoveWarningLeading.rawValue, time: 5.888000),
    .init(misc: Misc.WrongMoveWarningTrailing.rawValue, time: 9.632000),
    .init(misc: Misc.YourTurnBlack.rawValue, time: 10.240000),
    .init(misc: Misc.YourTurnWhite.rawValue, time: 11.765333),
    .init(misc: Misc.Aborted.rawValue, time: 13.162666),
    .init(misc: Misc.Error.rawValue, time: 14.677333),
    .init(misc: UInt32.max, time: 16.533333),
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

  func playYourTurn(_ color: sci.Color) {
    let offset = schedule(misc: color == sci.Color.Black ? .YourTurnBlack : .YourTurnWhite, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  func playNextMoveReady() {
    // ct-keytone2
    AudioServicesPlaySystemSound(1075)
  }

  func playAborted() {
    let offset = schedule(misc: .Aborted, offset: availableOffset)
    updateLatestAfter(seconds: offset)
  }

  func playError() {
    let offset = schedule(misc: .Error, offset: availableOffset)
    updateLatestAfter(seconds: offset)
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
    let suffixPosition = move.suffix & sci.SuffixType.MaskPosition.rawValue
    let suffixAction = move.suffix & sci.SuffixType.MaskAction.rawValue
    if suffixPosition == sci.SuffixType.Right.rawValue {
      actions.append(.Right)
    } else if suffixPosition == sci.SuffixType.Left.rawValue {
      actions.append(.Left)
    } else if suffixPosition == sci.SuffixType.Nearest.rawValue {
      actions.append(.Nearest)
    }
    if suffixAction == sci.SuffixType.Up.rawValue {
      actions.append(.Up)
    } else if suffixAction == sci.SuffixType.Down.rawValue {
      actions.append(.Down)
    } else if suffixAction == sci.SuffixType.Sideway.rawValue {
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
