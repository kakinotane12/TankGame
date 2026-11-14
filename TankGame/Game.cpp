# include "Game.h"

namespace
{
	const ColorF PlayerColor{ 0.30, 0.45, 0.25 };

	constexpr double ReadyTime = 1.5;    // 開始前の「READY」の表示時間 [秒]
	constexpr double AfterTime = 2.0;    // クリア / 撃破のあと、次へ進むまでの時間 [秒]

	// 効果音の音量（0.0 〜 1.0）
	constexpr double PlayerFireVolume = 0.35;
	constexpr double EnemyFireVolume = 0.2;
	constexpr double BounceVolume = 0.3;
	constexpr double ExplosionVolume = 0.8;

	// キーボードとマウスの状態から、プレイヤー戦車への操作指示を作る（Stage 2 で作ったもの）
	TankInput MakePlayerInput()
	{
		TankInput input;

		// W と S を同時に押したら打ち消し合って 0 になる
		if (KeyW.pressed())
		{
			input.move += 1.0;
		}
		if (KeyS.pressed())
		{
			input.move -= 1.0;
		}

		if (KeyD.pressed())
		{
			input.rotate += 1.0;
		}
		if (KeyA.pressed())
		{
			input.rotate -= 1.0;
		}

		input.aimTarget = Cursor::PosF();

		// 押しっぱなしで撃ち続けられるが、実際の発射間隔は Tank のクールダウンが決める。
		input.fire = MouseL.pressed();

		return input;
	}

	// 射撃のばらつきの範囲を、砲身の先から伸びる扇形（三角形）で描く（Stage 8）
	// length: 扇形の長さ [ピクセル]
	void DrawSpreadCone(const Tank& tank, const double length, const ColorF& color)
	{
		const Vec2 muzzle = tank.getMuzzlePos();
		const double angle = tank.getTurretAngle();
		const double spread = tank.getSpread();

		// 角度 a の向きに length だけ進んだ点
		const auto pointAt = [&](const double a) { return muzzle + Vec2{ Math::Cos(a), Math::Sin(a) } * length; };

		const Vec2 left = pointAt(angle - spread);
		const Vec2 right = pointAt(angle + spread);

		Triangle{ muzzle, left, right }.draw(color);
		Line{ muzzle, left }.draw(1.0, color.withAlpha(color.a * 3));
		Line{ muzzle, right }.draw(1.0, color.withAlpha(color.a * 3));
	}

	// マップの出現位置から敵を作る
	Array<Enemy> SpawnEnemies(const Map& map)
	{
		Array<Enemy> enemies;

		for (const auto& spawn : map.getEnemySpawns())
		{
			const ColorF color = (spawn.type == EnemyType::Chaser)
				? ColorF{ 0.60, 0.30, 0.25 }   // 追跡型: 赤
				: ColorF{ 0.45, 0.30, 0.55 };  // 固定砲台: 紫

			enemies.push_back(Enemy{ Tank{ spawn.pos, color, Team::Enemy }, MakeEnemyAI(spawn.type) });
		}

		return enemies;
	}
}

// ------------------------------------------------------------
// コンストラクタ
// ------------------------------------------------------------
// メンバ変数は「宣言した順」に初期化される（初期化子リストに書いた順ではない）。
// m_player は Tank なので、何かしらの値を渡して作る必要がある。
// マップを読み込むまで初期位置が分からないので、まず仮の位置で作り、読み込み後に置き換える。
Game::Game(const InitData& init)
	: IScene{ init }
	, m_player{ Vec2{ 0, 0 }, PlayerColor, Team::Player }
{
	const FilePath path = StageMapPath(getData().stage);

	if (not m_map.load(path))
	{
		System::MessageBoxOK(U"マップ {} を読み込めませんでした。"_fmt(path));

		// notifyError(): シーンマネージャーにエラーを伝える。manager.update() が false を返し、アプリが終了する。
		notifyError();
		return;
	}

	m_player = Tank{ m_map.getPlayerStart(), PlayerColor, Team::Player };
	m_enemies = SpawnEnemies(m_map);
}

// ------------------------------------------------------------
// 更新
// ------------------------------------------------------------
void Game::setPhase(const Phase phase)
{
	m_phase = phase;
	m_phaseTimer = 0.0;
}

void Game::onTankDestroyed(const Tank& tank)
{
	// IEffect の派生クラスを追加するときは、add<クラス>(コンストラクタの引数...) と書く
	m_effect.add<ExplosionEffect>(tank.getPos(), tank.getColor());

	AudioAsset(U"Explosion").playOneShot(ExplosionVolume);

	// プレイヤーがやられたときは大きく、敵を倒したときは小さく揺らす
	if (tank.getTeam() == Team::Player)
	{
		startShake(10.0, 0.5);
	}
	else
	{
		startShake(4.0, 0.25);
	}
}

void Game::startShake(const double power, const double time)
{
	if (m_shakePower * m_shakeTime < power * time)
	{
		m_shakePower = power;
		m_shakeTime = time;
	}
}

void Game::update()
{
	const double deltaTime = Scene::DeltaTime();
	m_phaseTimer += deltaTime;

	// --- 画面の揺れ ---
	// 残り時間に比例して弱くなるように、ランダムな向きにずらす。
	// ずれの量は update() で決めておき、draw() ではそれを使うだけにする（draw は const なので状態を変えられない）。
	m_shakeTime = Max(m_shakeTime - deltaTime, 0.0);
	m_shakeOffset = (0.0 < m_shakeTime) ? RandomVec2(m_shakePower * Min(m_shakeTime * 4.0, 1.0)) : Vec2{ 0, 0 };

	if (KeyF2.down())
	{
		m_showDebug = (not m_showDebug);
	}

	switch (m_phase)
	{
	case Phase::Ready:
		// 開始前は何も動かさない
		if (ReadyTime <= m_phaseTimer)
		{
			setPhase(Phase::Playing);
		}
		break;

	case Phase::Playing:
		updateWorld(deltaTime, true);

		if (not m_player.isAlive())
		{
			setPhase(Phase::Destroyed);
		}
		else if (m_enemies.isEmpty())
		{
			setPhase(Phase::Cleared);
		}
		break;

	case Phase::Cleared:
		// クリア後は弾だけ消えるまで飛ばしておく（プレイヤーは止める）
		updateWorld(deltaTime, false);

		if (AfterTime <= m_phaseTimer)
		{
			goToNextScene();
		}
		break;

	case Phase::Destroyed:
		// 撃破された後も、敵と弾はしばらく動き続ける
		updateWorld(deltaTime, false);

		if (AfterTime <= m_phaseTimer)
		{
			goToNextScene();
		}
		break;
	}
}

void Game::updateWorld(const double deltaTime, const bool playerControllable)
{
	// --- 戦車 ---
	if (playerControllable && m_player.isAlive())
	{
		m_player.update(MakePlayerInput(), deltaTime, m_map, m_bullets);

		if (m_player.firedThisFrame())
		{
			AddMuzzleFlash(m_effect, m_player.getMuzzlePos(), m_player.getTurretAngle());
			AudioAsset(U"Fire").playOneShot(PlayerFireVolume);
		}
	}

	for (auto& enemy : m_enemies)
	{
		const TankInput input = enemy.ai->think(enemy.tank, m_player, m_map, deltaTime);
		enemy.tank.update(input, deltaTime, m_map, m_bullets);

		if (enemy.tank.firedThisFrame())
		{
			AddMuzzleFlash(m_effect, enemy.tank.getMuzzlePos(), enemy.tank.getTurretAngle());
			AudioAsset(U"Fire").playOneShot(EnemyFireVolume);
		}
	}

	// --- 弾 ---
	for (auto& bullet : m_bullets)
	{
		bullet.update(deltaTime, m_map);

		if (bullet.bouncedThisFrame())
		{
			AddBounceSpark(m_effect, bullet.getPos());
			AudioAsset(U"Bounce").playOneShot(BounceVolume);
		}
	}

	// --- 弾と戦車の当たり判定 ---
	for (auto& bullet : m_bullets)
	{
		if (bullet.getTeam() == Team::Enemy)
		{
			if (m_player.isAlive() && m_player.getCircle().intersects(bullet.getCircle()))
			{
				m_player.destroy();
				bullet.destroy();
				onTankDestroyed(m_player);
			}
		}
		else
		{
			for (auto& enemy : m_enemies)
			{
				if (enemy.tank.isAlive() && enemy.tank.getCircle().intersects(bullet.getCircle()))
				{
					enemy.tank.destroy();
					bullet.destroy();
					onTankDestroyed(enemy.tank);
					break;
				}
			}
		}
	}

	// --- 消えるものを取り除く ---
	m_bullets.remove_if([](const Bullet& b) { return (not b.isAlive()); });
	m_enemies.remove_if([](const Enemy& e) { return (not e.tank.isAlive()); });
}

void Game::goToNextScene()
{
	if (m_sceneChanging)
	{
		return;
	}
	m_sceneChanging = true;

	GameData& data = getData();

	if (m_phase == Phase::Cleared)
	{
		if (data.stage < StageCount)
		{
			// 次のステージへ。同じ "Game" シーンに切り替えると、Game が新しく作り直される。
			++data.stage;
			changeScene(U"Game", 0.6s);
		}
		else
		{
			data.allCleared = true;
			changeScene(U"Result", 1.0s);
		}
	}
	else // Phase::Destroyed
	{
		--data.lives;

		if (0 < data.lives)
		{
			// 残機があれば、同じステージをやり直す
			changeScene(U"Game", 0.6s);
		}
		else
		{
			data.allCleared = false;
			changeScene(U"Result", 1.0s);
		}
	}
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
void Game::draw() const
{
	// --- ゲームの世界（マップ・戦車・弾・演出）は、揺れの分だけずらして描く ---
	// HUD やメッセージは揺らさないので、このブロックの外で描く。
	{
		const Transformer2D shake{ Mat3x2::Translate(m_shakeOffset) };

		m_map.draw();

		// 射撃のばらつきの表示（戦車より下に描いて、車体を隠さないようにする）
		if (m_player.isAlive() && (m_phase == Phase::Playing))
		{
			// 扇形の長さはカーソルまでの距離（近すぎると見えないので最低 80px）
			const double length = Max(m_player.getMuzzlePos().distanceFrom(Cursor::PosF()), 80.0);
			DrawSpreadCone(m_player, length, ColorF{ 1.0, 1.0, 1.0, 0.12 });
		}

		if (m_showDebug)
		{
			for (const auto& enemy : m_enemies)
			{
				DrawSpreadCone(enemy.tank, 200.0, ColorF{ 1.0, 0.2, 0.2, 0.10 });
			}
		}

		for (const auto& enemy : m_enemies)
		{
			enemy.tank.draw();
		}

		if (m_player.isAlive())
		{
			m_player.draw();
		}

		for (const auto& bullet : m_bullets)
		{
			bullet.draw();
		}

		// 追加されている演出をすべて描く（時間切れのものは自動で取り除かれる）
		m_effect.update();
	}

	// --- AI のデバッグ表示（F2） ---
	if (m_showDebug)
	{
		for (const auto& enemy : m_enemies)
		{
			const Vec2 pos = enemy.tank.getPos();

			if (m_player.isAlive() && m_map.canSee(pos, m_player.getPos()))
			{
				Line{ pos, m_player.getPos() }.draw(1.0, ColorF{ 1.0, 0.0, 0.0, 0.3 });
			}

			FontAsset(U"Small")(enemy.ai->getStateName()).drawAt(pos.movedBy(0, -32), ColorF{ 0.1 });
		}
	}

	// --- 照準（カーソルの位置）---
	// 円の半径 = 「カーソルの距離で、弾がどれだけ左右にずれうるか」= 距離 × tan(ばらつき)。
	// 遠くを狙うほど円が大きくなり、同じ角度のずれでも遠いほど外れやすいことが分かる。
	// 揺れの影響を受けないよう、Transformer2D のブロックの外で描く（カーソルは揺れないため）。
	if (m_player.isAlive() && (m_phase == Phase::Playing))
	{
		const Vec2 cursor = Cursor::PosF();
		const double distance = m_player.getMuzzlePos().distanceFrom(cursor);
		const double radius = Max(distance * Math::Tan(m_player.getSpread()), 4.0);

		Circle{ cursor, radius }.drawFrame(2.0, ColorF{ 1.0, 0.95, 0.6, 0.9 });
		Circle{ cursor, 2.0 }.draw(ColorF{ 1.0, 0.95, 0.6, 0.9 });
	}

	drawHUD();

	// --- フェーズに応じたメッセージ ---
	const GameData& data = getData();

	switch (m_phase)
	{
	case Phase::Ready:
		drawCenterMessage(U"STAGE {}"_fmt(data.stage), ColorF{ 0.95 });
		break;

	case Phase::Playing:
		break;

	case Phase::Cleared:
		drawCenterMessage(U"STAGE CLEAR!", ColorF{ 0.95, 0.85, 0.40 });
		break;

	case Phase::Destroyed:
		drawCenterMessage(U"DESTROYED", ColorF{ 0.90, 0.40, 0.35 });
		break;
	}
}

void Game::drawHUD() const
{
	const GameData& data = getData();

	// マップのいちばん上の行（外周の壁）に、半透明の帯を敷いて情報を表示する
	RectF{ 0, 0, Scene::Width(), Map::TileSize }.draw(ColorF{ 0.0, 0.5 });

	const Font& font = FontAsset(U"Medium");
	font(U"STAGE {} / {}"_fmt(data.stage, StageCount)).draw(Arg::leftCenter(16, Map::TileSize / 2), ColorF{ 0.95 });

	// 残機は戦車の色の小さな四角で表す
	font(U"残機").draw(Arg::leftCenter(260, Map::TileSize / 2), ColorF{ 0.95 });
	for (int32 i = 0; i < data.lives; ++i)
	{
		RectF{ Arg::center((320 + i * 24), Map::TileSize / 2), 18, 14 }.draw(PlayerColor).drawFrame(1, ColorF{ 0.1 });
	}

	// 今のばらつきを角度で表示する（Stage 8）。ToDegrees: ラジアン → 度
	font(U"精度 ±{:.1f}°"_fmt(Math::ToDegrees(m_player.getSpread()))).draw(Arg::leftCenter(480, Map::TileSize / 2), ColorF{ 0.95 });

	font(U"敵 {}"_fmt(m_enemies.size())).draw(Arg::rightCenter((Scene::Width() - 16), Map::TileSize / 2), ColorF{ 0.95 });
}

void Game::drawCenterMessage(const String& text, const ColorF& color) const
{
	RectF{ Arg::center(Scene::Center()), Scene::Width(), 100 }.draw(ColorF{ 0.0, 0.5 });
	FontAsset(U"Large")(text).drawAt(TextStyle::Outline(0.2, ColorF{ 0.1 }), 56, Scene::Center(), color);
}
