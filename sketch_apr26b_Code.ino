/**
 *
 * Този код изпълнява опростена версия на класическата игра Tetris, използвайки
 * платка Arduino и 16x2 LCD дисплей. Играта включва просто въведение
 * екран, система за точкуване и стандартната механика на играта Tetris
 * падащи тетромино и изчистващи линии.
 *
 */

#include <EEPROM.h>
#include <LiquidCrystal.h>

/* Settings */
constexpr unsigned long kIntroDelay{1000};          // default: 1000
constexpr unsigned long kGameOverDelay{3000};       // default: 3000
constexpr unsigned long kActionTimeDelay{150};      // default: 150
constexpr unsigned long kMoveTimeDelay{350};        // default: 350
constexpr unsigned long kRapidFallLineDelay{20};    // default: 20
constexpr unsigned long kClearedLineScoreBonus{5};  // default: 5

/* Pins */
constexpr int kLeftButtonPin{4};
constexpr int kRightButtonPin{3};
constexpr int kRotateButtonPin{5};
constexpr int kRapidFallButtonPin{2};
constexpr int kLcdRsPin{8};
constexpr int kLcdEnablePin{9};
constexpr int kLcdD4Pin{10};
constexpr int kLcdD5Pin{11};
constexpr int kLcdD6Pin{12};
constexpr int kLcdD7Pin{13};

/* Constants */
constexpr int kFigures[7][4]{{0, 2, 4, 6}, {2, 4, 5, 7}, {3, 5, 4, 6},
                             {3, 5, 4, 7}, {2, 3, 5, 7}, {3, 5, 7, 6},
                             {2, 3, 4, 5}};
constexpr int kFiguresSize{sizeof(kFigures) / sizeof(kFigures[0])};
constexpr int kFigureSize{sizeof(kFigures[0]) / sizeof(kFigures[0][0])};

/**
 * Класът Board се използва за съхраняване и управление на състоянието на таблото за игра.
 */
class Board {
 public:
  /**
   * @param x The x-coordinate of the position
   * @param y The y-coordinate of the position
   *
   * @returns Булевата стойност на дъската на посочената позиция.
   */
  bool At(const int x, const int y) const;
  /**
   * Задава стойността на дъската в указаната позиция на
   * определена булева стойност
   *
   * @param x The x-coordinate of the position
   * @param y The y-coordinate of the position
   * @param стойност Новата стойност в определена позиция
   */
  void Set(const int x, const int y, const bool value);
  /**
   * Проверява за пълни линии на дъската и ги изчиства, премествайки всички линии отгоре
   * ги надолу, ако е необходимо.
   *
   * @returns Броят на изчистените редове.
   */
  int ClearLines();
  /**
   * Изчиства дъската за игра, като задава всички стойности в масива на дъската на false.
   */
  void Clear();

  static constexpr int Width() { return kWidth; }
  static constexpr int Height() { return kHeight; }
  static constexpr int BlockWidth() { return kBlockWidth; }
  static constexpr int BlockHeight() { return kBlockHeight; }
  static constexpr int Columns() { return kWidth / kBlockWidth; }
  static constexpr int Rows() { return kHeight / kBlockHeight; }

 private:
  static constexpr int kWidth{16};
  static constexpr int kHeight{20};
  static constexpr int kBlockWidth{8};
  static constexpr int kBlockHeight{5};

  /**
   * Премества всички редове с y по-ниско от предоставената стойност с един надолу
   * блок на дъската. Този метод се използва за актуализиране на платката след изчистване
   * линии.
   *
   * @param max_y The y-coordinate стойност, под която редовете трябва да се преместят надолу
   */
  void MoveLines(const int max_y);

  bool board_[kWidth][kHeight];
};

bool Board::At(const int x, const int y) const { return board_[x][y]; }

void Board::Set(const int x, const int y, const bool value) {
  board_[x][y] = value;
}

int Board::ClearLines() {
  int cleared_lines{0};

  int y{Board::Height() - 1};
  while (y >= 0) {
    int quantity{0};
    for (int x{0}; x < Board::Width(); ++x) {
      if (At(x, y)) ++quantity;
    }

    if (quantity == 0) {
      break;
    } else if (quantity == Board::Width()) {
      for (int x{0}; x < Board::Width(); ++x) {
        Set(x, y, false);
      }

      MoveLines(y);
      ++cleared_lines;
    } else {
      --y;
    }
  }

  return cleared_lines;
}

void Board::Clear() {
  for (int x{0}; x < Board::Width(); ++x) {
    for (int y{0}; y < Board::Height(); ++y) {
      Set(x, y, false);
    }
  }
}

void Board::MoveLines(const int max_y) {
  for (int y{max_y}; y > 0; --y) {
    for (int x{0}; x < Board::Width(); ++x) {
      Set(x, y, At(x, y - 1));
    }
  }
}

/**
 * Класът Tetromino отговаря за създаването и манипулирането на Tetromino
 * обекти, които представляват различните форми, използвани в играта Tetris. А
 * Обектът Tetromino е съставен от четири блока, подредени в специфична форма,
 * като права линия или квадрат. Класът Tetromino предоставя функции
 * за преместване и завъртане на тези форми на игрална дъска.
 */
class Tetromino {
 public:
  /**
   * Преброяването Direction се използва за указване на посоката за движение на тетромино
   * хоризонтално. Има две възможни стойности, kLeft и kRight, които
   * съответно представляват преместване на тетроминото наляво или надясно.
   *
   * Преброяването Direction се използва във връзка с метода Move,
   * който премества тетроминото хоризонтално въз основа на указаната посока.
   */
  enum class Direction {
    kLeft = -1,
    kRight = 1,
  };

  /**
   * Конструкторът на клас Tetromino създава нов Tetromino обект на случаен принцип
   * фигура от набора от предварително дефинирани фигури.
   */
  Tetromino();

  /**
   * Проверява дали тетроминото се сблъсква с други блокове на игралната дъска.
   *
   * @param board Платката за проверка за сблъсъци с тетромина
   *
   * @returns True, ако е имало сблъсък, false в противен случай.
   */
  bool Collide(const Board& board) const;
  /**
   * Премества текущото тетромино наляво или надясно с една колона, ако е възможно.
   *
   * @param дъска Дъската за проверка за сблъсъци с тетромино след това
   * движение в определена посока
   * @param direction Посоката за движение на тетроминото
   * `Direction::kLeft`
   *
   * @returns True, ако преместването е станало, false в противен случай.
   */
  bool Move(const Board& board, const Direction direction);
  /**
   * Завърта тетроминото на 90 градуса, ако е възможно.
   *
   * @param дъска Дъската за проверка за сблъсъци с тетромино след това
   * завъртане
   *
   * @returns True, ако се е случило завъртане, false в противен случай.
   */
  bool Rotate(const Board& board);
  /**
   * Премества тетроминото с един ред надолу, ако е възможно.
   *
   * @param дъска Дъската за проверка за сблъсъци с тетромино след това
   * преместване надолу
   *
   * @returns True, ако тетроминото е на дъното, false в противен случай.
   */
  bool MoveDown(const Board& board);
  /**
   * Добавя или премахва тетромино от игралната дъска, в зависимост от стойността
   * на предадения аргумент.
   *
   * @param дъска Дъската за рисуване или премахване на блоковете тетромино
   * @param стойност Булевата стойност, където true показва, че тетромино
   * трябва да бъде нарисувано на дъската, а false показва, че трябва да бъде премахнато
   * от дъската
   */
  void Draw(Board& board, const bool value) const;

 private:
  /**
   * Блоковата структура представлява единичен блок от тетромино. Състои се от
   * две полета, x и y, които представляват x и y координатите на блока,
   * съответно.
   */
  struct Block {
    int x;
    int y;
  };

  Block shape_[kFigureSize];
};

Tetromino::Tetromino() {
  constexpr int x{Board::Width() / 2};
  constexpr int y{0};

  const int figure{random(kFiguresSize)};
  for (int i{0}; i < kFigureSize; ++i) {
    shape_[i].x = x + kFigures[figure][i] % 2;
    shape_[i].y = y + kFigures[figure][i] / 2;
  }
}

bool Tetromino::Collide(const Board& board) const {
  for (int i{0}; i < kFigureSize; ++i) {
    const Block& block{shape_[i]};
    if (board.At(block.x, block.y)) return true;
  }
  return false;
}

bool Tetromino::Move(const Board& board, const Direction direction) {
  const int offset{static_cast<int>(direction)};

  for (int i{0}; i < kFigureSize; ++i) {
    const int new_x{shape_[i].x + offset};
    if (new_x < 0 || new_x >= Board::Width()) return false;
    if (board.At(new_x, shape_[i].y)) return false;
  }

  for (int i{0}; i < kFigureSize; ++i) {
    shape_[i].x += offset;
  }
  return true;
}

bool Tetromino::Rotate(const Board& board) {
  // Disables square rotation
  if (abs(shape_[3].x - shape_[0].x) == 1 &&
      abs(shape_[3].y - shape_[0].y) == 1)
    return false;

  const Block& center = shape_[2];
  Block rotated[kFigureSize];

  for (int i{0}; i < kFigureSize; ++i) {
    const int x{shape_[i].y - center.y};
    const int y{shape_[i].x - center.x};
    rotated[i].x = center.x - x;
    rotated[i].y = center.y + y;

    if (rotated[i].x < 0 || rotated[i].x >= Board::Width()) return false;
    if (rotated[i].y < 0 || rotated[i].x >= Board::Height()) return false;
    if (board.At(rotated[i].x, rotated[i].y)) return false;
  }

  for (int i{0}; i < kFigureSize; ++i) {
    shape_[i].x = rotated[i].x;
    shape_[i].y = rotated[i].y;
  }
  return true;
}

bool Tetromino::MoveDown(const Board& board) {
  for (int i{0}; i < kFigureSize; ++i) {
    if (shape_[i].y >= Board::Height() - 1) return true;
    if (board.At(shape_[i].x, shape_[i].y + 1)) return true;
  }

  for (int i{0}; i < kFigureSize; ++i) {
    ++shape_[i].y;
  }
  return false;
}

void Tetromino::Draw(Board& board, const bool value) const {
  for (int i{0}; i < kFigureSize; ++i) {
    const Block& block = shape_[i];
    board.Set(block.x, block.y, value);
  }
}

/**
 * Класът Display е отговорен за управлението на дисплея на играта на
 * LCD екран. Той предоставя методи за отпечатване на различни съобщения, рисуване на
 * дъска за игра, форми на тетромино и информация за резултат на екрана.
 */
class Display {
 public:
  /**
   * Конструкторът приема шест аргумента, които са пин номерата за LCD
   * екран. Той създава обект LiquidCrystal отдолу, за да взаимодейства с
   * LCD екран.
   *
   * @param rs
   * @param enable
   * @param d4
   * @param d5
   * @param d6
   * @param d7
   */
  Display(int rs, int enable, int d4, int d5, int d6, int d7)
      : display_{rs, enable, d4, d5, d6, d7} {}

  /**
   * Чертае игралната дъска на дисплея, включително текущото тетромино, ако
   * на разположение.
   *
   * @param дъска Дъската за игра
   * @param тетромино Текущият тетромино (по избор)
   **/
  void DrawBoard(Board& board, const Tetromino* const tetromino);
  /**
   * Актуализира резултата от играта на дисплея.
   *
   * @param score Новият резултат за показване
   */
  void PrintScore(const int score);

  /**
   * Показва уводно съобщение, което включва заглавието на играта и автора.
   */
  void Intro();
  /**
   * Изчиства дисплея на играта и отпечатва резултата от играта, започвайки с резултат от
   * 0.
   */
  void Start();
  /**
   * Показва текущия резултат и високия резултат на играча на екрана на играта.
   *
   * @param score Текущият резултат на играта
   * @param high_score Най-висок резултат, постигнат във всички предишни игри
   */
  void GameOver(const int score, const int high_score);
  /**
   * Показва "game over" съобщение и предоставя инструкции за рестартиране
   * играта.
   */
  void Restart();

 private:
  /**
   * Актуализира героя за определен сегмент от дъската.
   *
   * @param колона Номерът на колоната на дисплея
   * @param ред Номерът на реда на дисплея
   * @param дъска Дъската за игра
   *
   * @returns True, ако са направени промени, false в противен случай.
   */
  bool UpdateCharacter(const int column, const int row, const Board& board);

  LiquidCrystal display_;
  uint8_t characters_[Board::Columns()][Board::Rows()][Board::BlockWidth()];
};

void Display::DrawBoard(Board& board, const Tetromino* const tetromino) {
  if (tetromino) tetromino->Draw(board, true);

  for (int column{0}; column < Board::Columns(); ++column) {
    for (int row{0}; row < Board::Rows(); ++row) {
      if (UpdateCharacter(column, row, board)) {
        const int index{column * Board::Rows() + row};
        display_.createChar(index, characters_[column][row]);
        display_.setCursor(row, column);
        display_.write(uint8_t(index));
      }
    }
  }

  if (tetromino) tetromino->Draw(board, false);
}

void Display::PrintScore(const int score) {
  display_.setCursor(9, 1);
  display_.print(min(score, 999));
}

void Display::Intro() {
  uint8_t block_character[] = {0b11111, 0b11111, 0b11111, 0b11111,
                               0b11111, 0b11111, 0b11111, 0b11111};
  display_.createChar(0, block_character);
  display_.begin(16, 2);
}

void Display::Start() {
  for (int column{0}; column < Board::Columns(); ++column) {
    for (int row{0}; row < Board::Rows(); ++row) {
      for (int y{0}; y < Board::BlockWidth(); ++y) {
        characters_[column][row][y] = 0b00000;
      }
    }
  }

  display_.clear();
  display_.setCursor(4, 0);
  display_.print("XXXX Score:");

  display_.setCursor(4, 1);
  display_.print("XXXX");
  PrintScore(0);
}

void Display::GameOver(const int score, const int high_score) {
  uint8_t crown_character[] = {0b00000, 0b00000, 0b00000, 0b10101,
                               0b11111, 0b11111, 0b11111, 0b00000};
  display_.createChar(0, crown_character);
  display_.clear();

  display_.setCursor(0, 0);
  display_.print("Your score: ");
  display_.print(score);
  if (score >= high_score) display_.write(uint8_t(0));

  display_.setCursor(0, 1);
  display_.print("High score: ");
  display_.print(high_score);
  if (score <= high_score) display_.write(uint8_t(0));
}

void Display::Restart() {
  display_.clear();
  display_.setCursor(0, 0);
  display_.print("Game over! Press");
  display_.setCursor(0, 1);
  display_.print("rotate to play.");
}

bool Display::UpdateCharacter(const int column, const int row,
                              const Board& board) {
  const int x{column * Board::BlockWidth()};
  const int y{(Board::Rows() - row - 1) * Board::BlockHeight()};

  uint8_t character[8];
  constexpr uint8_t kPowers[]{1, 2, 4, 8, 16};

  for (int i{0}; i < Board::BlockWidth(); ++i) {
    uint8_t value{0};
    for (int j{0}; j < Board::BlockHeight(); ++j) {
      value += board.At(x + i, y + j) * kPowers[j];
    }
    character[i] = value;
  }

  bool changes{false};
  for (int i{0}; i < Board::BlockWidth(); ++i) {
    if (characters_[column][row][i] != character[i]) {
      characters_[column][row][i] = character[i];
      changes = true;
    }
  }

  return changes;
}

/**
 * Отговаря за инициализиране и управление на състоянието на играта. То също
 * обработва въведеното от потребителя и съответно актуализира състоянието на играта.
 */
class Game {
 public:
  /**
   * Освобождава паметта, използвана от обектите дисплей и тетромино.
   */
  ~Game();

  /**
   * Инициализира щифтовете за въвеждане, настройва EEPROM
   * за четене и писане на данни за висок резултат, инициализира дисплея за
   * изход и започва интро последователността.
   */
  void Setup();
  /**
   * Извършва актуализация на състоянието на играта всеки тик, което включва
   * актуализиране на позицията на тетромино, проверка за завършени линии,
   * и обработка на въвеждане от потребителя.
   */
  void Update();

 private:
  static constexpr int kMemoryHighScoreAddress{0};

  /**
   * Справя се с бързо падане на тетромино, ако потребителят избере да го направи. Ако потребителят
   * натиска клавиша "drop", методът незабавно премества текущото тетромино
   * в долната част на дъската, след което изчиства всички завършени редове и актуализации
   * резултат.
   *
   *  @returns Вярно, ако се случи бързо падане, невярно в противен случай.
   */
  bool HandleRapidFall();
  /**
   * Проверява въведеното от потребителя и актуализира tetromino, ако е възможно.
   *
   * @param time Изминалото време в милисекунди
   *
   * @returns True, ако е довело до промени в таблото, false в противен случай.
   */
  bool HandleUserInput(const unsigned long time);
  /**
   * Премества текущия тетромино с една клетка надолу, ако делта времето
   * тъй като последното движение надолу е по-голямо от постоянна стойност.
   *
   * @param time Изминалото време в милисекунди
   *
   * @returns True, ако е довело до промени в таблото, false в противен случай.
   */
  bool HandleTetrominoMoveDown(const unsigned long time);
  /**
   * Премахва текущото тетромино от дъската и актуализира резултата.
   */
  void RemoveTetromino();

  /**
   * Показва интро последователността на играта, която включва показване на играта
   * заглавие и автор. След като интрото приключи, методът стартира играта.
   */
  void Intro();
  /**
   * Нулира състоянието на играта и стартира играта. Това включва нулиране
   * резултатът, изтриване на тетромино и изчистване на игралната дъска.
   */
  void Start();
  /**
   * Показва съобщението „играта приключи“ и изчаква играчът да рестартира
   * игра. Ако играчът е постигнал нов висок резултат, методът го запазва в
   * EEPROM паметта.
   */
  void GameOver();

  Board board_;
  Display* display_;
  Tetromino* tetromino_;

  int score_{0};
  unsigned long last_action_time_{0};
  unsigned long last_move_time_{0};
};

Game::~Game() {
  delete display_;
  delete tetromino_;
}

void Game::Setup() {
  pinMode(kLeftButtonPin, INPUT_PULLUP);
  pinMode(kRightButtonPin, INPUT_PULLUP);
  pinMode(kRotateButtonPin, INPUT_PULLUP);
  pinMode(kRapidFallButtonPin, INPUT_PULLUP);

  randomSeed(analogRead(0));

  if (!EEPROM.read(kMemoryHighScoreAddress)) {
    EEPROM.write(kMemoryHighScoreAddress, 0);
  }

  display_ = new Display(kLcdRsPin, kLcdEnablePin, kLcdD4Pin, kLcdD5Pin,
                         kLcdD6Pin, kLcdD7Pin);

  Intro();
}

void Game::Update() {
  bool changes{false};

  if (!tetromino_) {
    tetromino_ = new Tetromino();
    changes = true;

    if (tetromino_->Collide(board_)) return GameOver();
  }

  const unsigned long time{millis()};
  if (HandleRapidFall()) return;
  if (HandleUserInput(time)) changes = true;
  if (HandleTetrominoMoveDown(time)) changes = true;

  if (changes) display_->DrawBoard(board_, tetromino_);
}

bool Game::HandleRapidFall() {
  if (digitalRead(kRapidFallButtonPin) == LOW) {
    while (!tetromino_->MoveDown(board_)) {
      display_->DrawBoard(board_, tetromino_);
      delay(kRapidFallLineDelay);
    }

    RemoveTetromino();
    return true;
  }

  return false;
}

bool Game::HandleUserInput(const unsigned long time) {
  const unsigned long last_action_delta{time - last_action_time_};

  if (last_action_delta >= kActionTimeDelay) {
    bool action{false};

    if (digitalRead(kLeftButtonPin) == LOW) {
      action = tetromino_->Move(board_, Tetromino::Direction::kLeft);
    } else if (digitalRead(kRightButtonPin) == LOW) {
      action = tetromino_->Move(board_, Tetromino::Direction::kRight);
    } else if (digitalRead(kRotateButtonPin) == LOW) {
      action = tetromino_->Rotate(board_);
    }

    if (action) {
      last_action_time_ = time;
      return true;
    }
  }

  return false;
}

bool Game::HandleTetrominoMoveDown(const unsigned long time) {
  const unsigned long last_move_delta{time - last_move_time_};

  if (last_move_delta >= kMoveTimeDelay) {
    if (tetromino_->MoveDown(board_)) RemoveTetromino();
    last_move_time_ = time;
    return true;
  }

  return false;
}

void Game::RemoveTetromino() {
  tetromino_->Draw(board_, true);
  delete tetromino_;
  tetromino_ = nullptr;

  ++score_;
  score_ += board_.ClearLines() * kClearedLineScoreBonus;

  display_->PrintScore(score_);
}

void Game::Intro() {
  display_->Intro();
  delay(kIntroDelay);
  Start();
}

void Game::Start() {
  delete tetromino_;
  tetromino_ = nullptr;

  score_ = 0;
  last_action_time_ = millis();
  last_move_time_ = millis();

  board_.Clear();
  display_->Start();
}

void Game::GameOver() {
  const int high_score{EEPROM.read(0)};

  display_->GameOver(score_, high_score);
  if (score_ > high_score) EEPROM.update(0, score_);
  delay(kGameOverDelay);

  display_->Restart();
  while (digitalRead(kRotateButtonPin) == HIGH) {
  }
  Start();
}

Game game;

void setup() { game.Setup(); }

void loop() { game.Update(); }