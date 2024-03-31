static Game::ApplyResult MustMove(std::string const &msg, Game &g) {
  auto mv = MoveFromCsaMove(msg, g.position);
  REQUIRE(std::holds_alternative<Move>(mv));
  auto m = std::get<Move>(mv);
  return g.apply(m);
}

TEST_CASE("game") {
  SUBCASE("apply") {
    SUBCASE("千日手") {
      Game g(Handicap::平手, false);
      CHECK(MustMove("+2838HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-8272HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+3828HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-7282HI", g) == Game::ApplyResult::Ok);

      CHECK(MustMove("+2838HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-8272HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+3828HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-7282HI", g) == Game::ApplyResult::Ok);

      CHECK(MustMove("+2838HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-8272HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+3828HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-7282HI", g) == Game::ApplyResult::Repetition);
    }
    SUBCASE("連続王手の千日手") {
      Game g(Handicap::平手, false);
      CHECK(MustMove("+5756FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-5354FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+5655FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-5455FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+4746FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-4344FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+4645FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-4445FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+2848HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-5142OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+4845HI", g) == Game::ApplyResult::Ok);

      CHECK(MustMove("-4252OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+4555HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-5242OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+5545HI", g) == Game::ApplyResult::Ok);

      CHECK(MustMove("-4252OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+4555HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-5242OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+5545HI", g) == Game::ApplyResult::Ok);

      CHECK(MustMove("-4252OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+4555HI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-5242OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+5545HI", g) == Game::ApplyResult::Ok);

      CHECK(MustMove("-4252OU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+4555HI", g) == Game::ApplyResult::CheckRepetitionBlack);
    }
    SUBCASE("王手放置") {
      Game g(Handicap::平手, false);
      CHECK(MustMove("+9796FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-9192KY", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+8897KA", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-1112KY", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+9753UM", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-1314FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+5352UM", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-6172KI", g) == Game::ApplyResult::Illegal);
    }
    SUBCASE("二歩") {
      Game g(Handicap::平手, false);
      CHECK(MustMove("+9796FU", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-9192KY", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+8897KA", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-1112KY", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+9753UM", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("-7172GI", g) == Game::ApplyResult::Ok);
      CHECK(MustMove("+0056FU", g) == Game::ApplyResult::Illegal);
    }
  }
}
