#include <Arduino.h>
#include <M5Cardputer.h>
#include <vector>
#include <algorithm>

enum class RunState { Ready, Running, GameOver };
enum class ObType { CactusSmall, CactusTall, BirdLow, BirdMid };

struct Obstacle {
  ObType type;
  float x, y, w, h;
  bool passedBonus = false;
};

struct Player {
  float x = 24.0f;
  float y = 0.0f;
  float vy = 0.0f;
  bool grounded = true;
  bool ducking = false;
  uint8_t runFrame = 0;
  float animT = 0.0f;
};

static constexpr float GROUND_Y = 108.0f;
static constexpr float PLAYER_W_STAND = 16.0f;
static constexpr float PLAYER_H_STAND = 16.0f;
static constexpr float PLAYER_W_DUCK  = 16.0f;
static constexpr float PLAYER_H_DUCK  = 12.0f;

static constexpr float GRAVITY = 0.80f;
static constexpr float JUMP_VELOCITY = -10.5f;
static constexpr float MAX_FALL_SPEED = 13.5f;

static constexpr float BASE_SCROLL_SPEED = 3.4f;
static constexpr float MAX_SCROLL_SPEED = 12.8f;
static constexpr float SPEED_GAIN_PER_SEC = 0.09f;

static constexpr uint16_t COLOR_BG = 0xFFFF;
static constexpr uint16_t COLOR_GROUND = 0x7BEF;
static constexpr uint16_t COLOR_PLAYER = 0x0000;
static constexpr uint16_t COLOR_OBSTACLE = 0x0000;
static constexpr uint16_t COLOR_TEXT = 0x0000;

// Fixed timestep smoothing
static constexpr float FIXED_DT = 1.0f / 60.0f;
static constexpr int MAX_STEPS = 4;

// Offscreen frame buffer (double buffering)
static LGFX_Sprite gFrame(&M5Cardputer.Display);
static bool gFrameReady = false;

// Bitmaps
static const uint8_t SPR_DINO_RUN_A_16x16[] PROGMEM = {
  0b00000000,0b00000000,0b00000111,0b00000000,0b00001111,0b10000000,0b00011111,0b11000000,
  0b00011111,0b11000000,0b00011111,0b11100000,0b00011111,0b11100000,0b00011111,0b00000000,
  0b00011111,0b10000000,0b00011111,0b11000000,0b00011111,0b11100000,0b00011111,0b11110000,
  0b00011011,0b01100000,0b00011000,0b01100000,0b00001100,0b11000000,0b00000000,0b00000000
};
static const uint8_t SPR_DINO_RUN_B_16x16[] PROGMEM = {
  0b00000000,0b00000000,0b00000111,0b00000000,0b00001111,0b10000000,0b00011111,0b11000000,
  0b00011111,0b11000000,0b00011111,0b11100000,0b00011111,0b11100000,0b00011111,0b00000000,
  0b00011111,0b10000000,0b00011111,0b11000000,0b00011111,0b11100000,0b00011111,0b11110000,
  0b00001101,0b10000000,0b00000110,0b11000000,0b00001100,0b11000000,0b00000000,0b00000000
};
static const uint8_t SPR_DINO_JUMP_16x16[] PROGMEM = {
  0b00000000,0b00000000,0b00000111,0b00000000,0b00001111,0b10000000,0b00011111,0b11000000,
  0b00011111,0b11000000,0b00011111,0b11100000,0b00011111,0b11100000,0b00011111,0b00000000,
  0b00011111,0b10000000,0b00011111,0b11000000,0b00011111,0b11100000,0b00011111,0b11110000,
  0b00011100,0b11100000,0b00011100,0b11100000,0b00001100,0b11000000,0b00000000,0b00000000
};
static const uint8_t SPR_DINO_DUCK_16x12[] PROGMEM = {
  0b00000000,0b00000000,0b00011111,0b11000000,0b00111111,0b11100000,0b00111111,0b11100000,
  0b00111111,0b11110000,0b00111111,0b11110000,0b00111111,0b11100000,0b00111111,0b11000000,
  0b00111110,0b00000000,0b00110110,0b11000000,0b00011001,0b10000000,0b00000000,0b00000000
};
static const uint8_t SPR_CACTUS_SMALL_12x18[] PROGMEM = {
  0b00000011,0b00000000,0b00000011,0b00000000,0b00000011,0b00000000,0b00000011,0b00000000,
  0b00000011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,
  0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,
  0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,
  0b00011111,0b10000000,0b00011111,0b10000000
};
static const uint8_t SPR_CACTUS_TALL_12x24[] PROGMEM = {
  0b00000011,0b00000000,0b00000011,0b00000000,0b00000011,0b00000000,0b00000011,0b00000000,
  0b00000011,0b00000000,0b00000011,0b00000000,0b00000011,0b00000000,0b00011011,0b00000000,
  0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,
  0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,
  0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,0b00011011,0b00000000,
  0b00011011,0b00000000,0b00011011,0b00000000,0b00011111,0b10000000,0b00011111,0b10000000
};
static const uint8_t SPR_BIRD_UP_14x8[] PROGMEM = {
  0b00001110,0b00000000,0b00011111,0b00000000,0b00111111,0b10000000,0b01111111,0b11000000,
  0b00111111,0b10000000,0b00011111,0b00000000,0b00001110,0b00000000,0b00000100,0b00000000
};
static const uint8_t SPR_BIRD_DOWN_14x8[] PROGMEM = {
  0b00000100,0b00000000,0b00001110,0b00000000,0b00011111,0b00000000,0b00111111,0b10000000,
  0b01111111,0b11000000,0b00111111,0b10000000,0b00011111,0b00000000,0b00001110,0b00000000
};

static bool keyPressedCompat(char c) {
  auto ks = M5Cardputer.Keyboard.keysState();
  for (auto &k : ks.word) if (k == c) return true;
  return false;
}
static bool jumpPressed() {
  return keyPressedCompat(' ') || keyPressedCompat('\n') ||
         keyPressedCompat('\r') || keyPressedCompat(';');
}
static bool duckHeld() {
  return keyPressedCompat(',') || keyPressedCompat('/') ||
         keyPressedCompat('d') || keyPressedCompat('D');
}

template <typename TD>
static void drawBitmap1bpp(TD &d, int x, int y, int w, int h,
                           const uint8_t* bmp, uint16_t fg, uint16_t bg, bool transparentBG=true) {
  int bytesPerRow = (w + 7) / 8;
  for (int yy = 0; yy < h; ++yy) {
    for (int xx = 0; xx < w; ++xx) {
      int byteIndex = yy * bytesPerRow + (xx >> 3);
      uint8_t b = pgm_read_byte(&bmp[byteIndex]);
      bool on = b & (0x80 >> (xx & 7));
      if (on) d.drawPixel(x + xx, y + yy, fg);
      else if (!transparentBG) d.drawPixel(x + xx, y + yy, bg);
    }
  }
}

class Game {
public:
  void reset() {
    _state = RunState::Ready;
    _p = Player{};
    _p.y = GROUND_Y - PLAYER_H_STAND;
    _scroll = BASE_SCROLL_SPEED;
    _score = 0;
    _phase = 1;
    _birdWingToggle = false;
    _birdAnimT = 0.0f;
    _obs.clear();
    spawnNext((float)M5Cardputer.Display.width() + 30.0f);
    ensureQueue();
  }

  RunState state() const { return _state; }

  void update(float dt, bool jmp, bool duck) {
    if (_state == RunState::GameOver) return;

    if (_state == RunState::Ready) {
      if (jmp) { _state = RunState::Running; _p.vy = JUMP_VELOCITY; _p.grounded = false; }
      return;
    }

    _p.ducking = duck && _p.grounded;
    if (jmp && _p.grounded && !_p.ducking) { _p.vy = JUMP_VELOCITY; _p.grounded = false; }

    _p.vy += GRAVITY;
    if (_p.vy > MAX_FALL_SPEED) _p.vy = MAX_FALL_SPEED;
    _p.y += _p.vy;

    float ph = (_p.ducking ? PLAYER_H_DUCK : PLAYER_H_STAND);
    float gy = GROUND_Y - ph;
    if (_p.y >= gy) { _p.y = gy; _p.vy = 0; _p.grounded = true; }

    _p.animT += dt;
    if (_p.animT >= 0.10f) { _p.animT = 0.0f; _p.runFrame ^= 1; }

    _birdAnimT += dt;
    if (_birdAnimT >= 0.14f) { _birdAnimT = 0.0f; _birdWingToggle = !_birdWingToggle; }

    _scroll += SPEED_GAIN_PER_SEC * dt;
    if (_scroll > MAX_SCROLL_SPEED) _scroll = MAX_SCROLL_SPEED;
    _phase = 1 + (uint8_t)(_score / 150);

    for (auto &o : _obs) o.x -= _scroll;
    for (auto &o : _obs) {
      if (!o.passedBonus && (o.x + o.w) < _p.x) { o.passedBonus = true; _score += 3; }
    }

    _obs.erase(std::remove_if(_obs.begin(), _obs.end(),
      [](const Obstacle& o){ return o.x + o.w < -2.0f; }), _obs.end());

    ensureQueue();

    float pw = (_p.ducking ? PLAYER_W_DUCK : PLAYER_W_STAND);
    float ph2 = (_p.ducking ? PLAYER_H_DUCK : PLAYER_H_STAND);
    for (const auto &o : _obs) {
      float px1 = _p.x, py1 = _p.y, px2 = _p.x + pw, py2 = _p.y + ph2;
      float ox1 = o.x, oy1 = o.y, ox2 = o.x + o.w, oy2 = o.y + o.h;
      if (px1 < ox2 && px2 > ox1 && py1 < oy2 && py2 > oy1) {
        _state = RunState::GameOver;
        if (_score > _hi) _hi = _score;
        return;
      }
    }

    _score += (uint32_t)(dt * 12.0f);
  }

  void render() const {
    if (gFrameReady) {
      drawScene(gFrame);
      gFrame.pushSprite(0, 0);
    } else {
      drawScene(M5Cardputer.Display);
    }
  }

private:
  template <typename TD>
  void drawScene(TD &d) const {
    // Clear playfield in one pass (prevents white artifact trails)
    d.fillRect(0, 0, d.width(), (int)GROUND_Y + 2, COLOR_BG);

    d.drawFastHLine(0, (int)GROUND_Y, d.width(), COLOR_GROUND);
    d.drawFastHLine(0, (int)GROUND_Y + 1, d.width(), COLOR_GROUND);

    int px = (int)_p.x, py = (int)_p.y;
    if (!_p.grounded) drawBitmap1bpp(d, px, py, 16, 16, SPR_DINO_JUMP_16x16, COLOR_PLAYER, COLOR_BG, true);
    else if (_p.ducking) drawBitmap1bpp(d, px, py, 16, 12, SPR_DINO_DUCK_16x12, COLOR_PLAYER, COLOR_BG, true);
    else if (_p.runFrame == 0) drawBitmap1bpp(d, px, py, 16, 16, SPR_DINO_RUN_A_16x16, COLOR_PLAYER, COLOR_BG, true);
    else drawBitmap1bpp(d, px, py, 16, 16, SPR_DINO_RUN_B_16x16, COLOR_PLAYER, COLOR_BG, true);

    for (const auto &o : _obs) {
      switch (o.type) {
        case ObType::CactusSmall:
          drawBitmap1bpp(d, (int)o.x, (int)o.y, 12, 18, SPR_CACTUS_SMALL_12x18, COLOR_OBSTACLE, COLOR_BG, true); break;
        case ObType::CactusTall:
          drawBitmap1bpp(d, (int)o.x, (int)o.y, 12, 24, SPR_CACTUS_TALL_12x24, COLOR_OBSTACLE, COLOR_BG, true); break;
        case ObType::BirdLow:
        case ObType::BirdMid:
          drawBitmap1bpp(d, (int)o.x, (int)o.y, 14, 8, _birdWingToggle ? SPR_BIRD_UP_14x8 : SPR_BIRD_DOWN_14x8, COLOR_OBSTACLE, COLOR_BG, true); break;
      }
    }

    d.setTextColor(COLOR_TEXT, COLOR_BG);
    d.setTextSize(1);
    d.setCursor(4, 4);   d.printf("@Kindle15");
    d.setCursor(4, 16);  d.printf("P%u", _phase);
    d.setCursor(64, 4);  d.printf("S %lu", (unsigned long)_score);
    d.setCursor(64, 16); d.printf("H %lu", (unsigned long)_hi);

    auto centered = [&](const String& s, int y, int size=1){
      d.setTextSize(size);
      int w = d.textWidth(s);
      int x = (d.width() - w) / 2;
      if (x < 0) x = 0;
      d.setCursor(x, y); d.print(s);
    };

    if (_state == RunState::Ready) {
      centered("DINO", 36, 2);
      centered("SPACE/ENTER jump", 58, 1);
      centered(", or / duck", 70, 1);
    } else if (_state == RunState::GameOver) {
      centered("GAME OVER", 36, 2);
      centered("SPACE/ENTER restart", 60, 1);
    }
  }

  Obstacle makeObstacle(float x) const {
    Obstacle o{}; o.x = x; int roll = random(0, 100);
    if (_phase <= 2) {
      if (roll < 70) { o.type = ObType::CactusSmall; o.w = 12; o.h = 18; o.y = GROUND_Y - o.h; }
      else           { o.type = ObType::CactusTall;  o.w = 12; o.h = 24; o.y = GROUND_Y - o.h; }
      return o;
    }
    if (roll < 42)      { o.type = ObType::CactusSmall; o.w = 12; o.h = 18; o.y = GROUND_Y - o.h; }
    else if (roll < 74) { o.type = ObType::CactusTall;  o.w = 12; o.h = 24; o.y = GROUND_Y - o.h; }
    else if (roll < 88) { o.type = ObType::BirdLow;     o.w = 14; o.h = 8;  o.y = GROUND_Y - 10; }
    else                { o.type = ObType::BirdMid;     o.w = 14; o.h = 8;  o.y = GROUND_Y - 26; }
    return o;
  }

  void spawnNext(float x) { _obs.push_back(makeObstacle(x)); }

  void ensureQueue() {
    if (_obs.empty()) { spawnNext((float)M5Cardputer.Display.width() + 20.0f); return; }
    while (_obs.size() < 4) {
      const auto &last = _obs.back();
      float baseGap = 85.0f - (_phase * 2.0f);
      if (baseGap < 58.0f) baseGap = 58.0f;
      float gap = baseGap + (float)random(0, 80);
      spawnNext(last.x + last.w + gap);
    }
  }

private:
  RunState _state = RunState::Ready;
  Player _p{};
  float _scroll = BASE_SCROLL_SPEED;
  uint32_t _score = 0, _hi = 0;
  uint8_t _phase = 1;
  std::vector<Obstacle> _obs;

  bool _birdWingToggle = false;
  float _birdAnimT = 0.0f;
};

Game g;

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextFont(1);
  randomSeed((uint32_t)esp_random());

  gFrame.setColorDepth(16);
  gFrameReady = gFrame.createSprite(M5Cardputer.Display.width(), M5Cardputer.Display.height());

  g.reset();
}

void loop() {
  M5Cardputer.update();

  static uint32_t prevMs = millis();
  static float accumulator = 0.0f;

  uint32_t now = millis();
  float frameDt = (now - prevMs) / 1000.0f;
  prevMs = now;

  if (frameDt < 0.0f) frameDt = 0.0f;
  if (frameDt > 0.1f) frameDt = 0.1f;
  accumulator += frameDt;

  bool jp = jumpPressed();
  bool dk = duckHeld();

  int steps = 0;
  while (accumulator >= FIXED_DT && steps < MAX_STEPS) {
    if (g.state() == RunState::GameOver && jp) {
      g.reset();
      g.update(0.0f, true, false);
    } else {
      g.update(FIXED_DT, jp, dk);
    }
    accumulator -= FIXED_DT;
    steps++;
  }

  if (steps == MAX_STEPS && accumulator > FIXED_DT) accumulator = 0.0f;

  g.render();
  delay(1);
}