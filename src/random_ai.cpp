#include <shogi_camera/shogi_camera.hpp>

namespace sci {

RandomAI::RandomAI() {
  std::random_device seed_gen;
  engine = std::make_unique<std::mt19937_64>(seed_gen());
}

std::optional<Move> RandomAI::next(Position const &p, std::vector<Move> const &mvs, std::deque<PieceType> const &hand, std::deque<PieceType> const &handEnemy) {
  std::deque<Move> moves;
  Color color = mvs.size() % 2 == 0 ? Color::Black : Color::White;
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
