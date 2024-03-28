static Move MustMove(std::string const &msg, Game &g) {
  auto mv = MoveFromCsaMove(msg, g.position);
  REQUIRE(std::holds_alternative<Move>(mv));
  return std::get<Move>(mv);
}

TEST_CASE("game") {
  SUBCASE("apply") {
    SUBCASE("千日手") {
      Game g;
      CHECK(g.apply(MustMove("+2838HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-8272HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+3828HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-7282HI", g)) == Game::ApplyResult::Ok);

      CHECK(g.apply(MustMove("+2838HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-8272HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+3828HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-7282HI", g)) == Game::ApplyResult::Ok);

      CHECK(g.apply(MustMove("+2838HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-8272HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+3828HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-7282HI", g)) == Game::ApplyResult::Repetition);
    }
    SUBCASE("連続王手の千日手") {
      Game g;
      CHECK(g.apply(MustMove("+5756FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-5354FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+5655FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-5455FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+4746FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-4344FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+4645FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-4445FU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+2848HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-5142OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+4845HI", g)) == Game::ApplyResult::Ok);

      CHECK(g.apply(MustMove("-4252OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+4555HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-5242OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+5545HI", g)) == Game::ApplyResult::Ok);

      CHECK(g.apply(MustMove("-4252OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+4555HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-5242OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+5545HI", g)) == Game::ApplyResult::Ok);

      CHECK(g.apply(MustMove("-4252OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+4555HI", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("-5242OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+5545HI", g)) == Game::ApplyResult::Ok);

      CHECK(g.apply(MustMove("-4252OU", g)) == Game::ApplyResult::Ok);
      CHECK(g.apply(MustMove("+4555HI", g)) == Game::ApplyResult::CheckRepetitionBlack);
    }
  }
}
