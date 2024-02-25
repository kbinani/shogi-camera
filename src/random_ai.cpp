#include <shogi_camera_input/shogi_camera_input.hpp>

namespace sci {

RandomAI::RandomAI(Color color) : AI(color) {
  std::random_device seed_gen;
  engine = std::make_unique<std::mt19937_64>(seed_gen());
}

std::optional<Move> RandomAI::next(Position const &p, std::vector<Move> const &, std::deque<PieceType> const &hand, std::deque<PieceType> const &handEnemy) {
  std::deque<Move> moves;
  Game::Generate(p, color, color == Color::Black ? hand : handEnemy, color == Color::Black ? handEnemy : hand, moves, true);
  if (moves.empty()) {
    return std::nullopt;
  } else if (moves.size() == 1) {
    return moves[0];
  } else {
    std::uniform_int_distribution<int> dist(0, (int)moves.size() - 1);
    int index = dist(*engine);
    return moves[index];
  }
}

} // namespace sci
