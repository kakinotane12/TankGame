# include <Siv3D.hpp> // Siv3D v0.6.16
# include "Tank.h"     // 自分で作ったヘッダーは "" で囲む（<> は外部ライブラリ）
# include "EnemyAI.h"

//
// Stage 5: 敵 AI と撃破
//
// 操作:
//   W / S : 前進 / 後退
//   A / D : 車体を左回転 / 右回転
//   マウス : 砲塔の向き
//   左クリック : 発射
//   R : やり直し
//   F2 : AI のデバッグ表示（状態と視線）の切り替え（F1 は Siv3D がライセンス表示に使っているので避ける）
//
// 弾が当たると1発で撃破。敵を全滅させればクリア。
// 赤い敵（マップの 'E'）は巡回 → 発見したら追跡 → 見失ったら捜索、紫の敵（'S'）は動かない固定砲台。
//

// ------------------------------------------------------------
// キーボードとマウスの状態から、プレイヤー戦車への操作指示を作る
// ------------------------------------------------------------
// 「入力を読む処理」を Tank の外に置くことで，クラス内には処理だけ記載すればよくなる．
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

	// pressed() は「押されている間ずっと true」。
	// 押しっぱなしで撃ち続けられるが、実際の発射間隔は Tank のクールダウンが決める。
	input.fire = MouseL.pressed();

	return input;
}

// ------------------------------------------------------------
// 敵 = 戦車（体）+ AI（頭脳）
// ------------------------------------------------------------
// AI は種類によって ChaserAI だったり SentryAI だったりするので、
// 「EnemyAI へのポインタ」として持つ。実体は std::unique_ptr が所有し、Enemy が消えると AI も消える。
struct Enemy
{
	Tank tank;
	std::unique_ptr<EnemyAI> ai;
};

// マップの出現位置から敵を作る
Array<Enemy> SpawnEnemies(const Map& map)
{
	Array<Enemy> enemies;

	for (const auto& spawn : map.getEnemySpawns())
	{
		const ColorF color = (spawn.type == EnemyType::Chaser)
			? ColorF{ 0.60, 0.30, 0.25 }   // 追跡型: 赤
			: ColorF{ 0.45, 0.30, 0.55 };  // 固定砲台: 紫

		// Enemy{ ... } で作った一時オブジェクトを、配列の末尾に「ムーブ（移動）」して入れる。
		// unique_ptr はコピーできない（所有者は1人だけ）ので、ムーブしかできない。
		enemies.push_back(Enemy{ Tank{ spawn.pos, color, Team::Enemy }, MakeEnemyAI(spawn.type) });
	}

	return enemies;
}

void Main()
{
	Window::SetTitle(U"TankGame - Stage 5");
	Scene::SetBackground(ColorF{ 0.85, 0.80, 0.65 });

	// ------------------------------------------------------------
	// マップを読み込む
	// ------------------------------------------------------------
	// パスは App フォルダーからの相対パス（実行時の作業フォルダーが App のため）
	Map map;
	if (not map.load(U"map/stage1.txt"))
	{
		// 読み込みに失敗したら、メッセージを出して終了する
		System::MessageBoxOK(U"マップ map/stage1.txt を読み込めませんでした。");
		return;
	}

	// ------------------------------------------------------------
	// ゲームの状態
	// ------------------------------------------------------------
	const ColorF playerColor{ 0.30, 0.45, 0.25 };
	Tank player{ map.getPlayerStart(), playerColor, Team::Player };
	Array<Enemy> enemies = SpawnEnemies(map);
	Array<Bullet> bullets;

	// デバッグ表示用
	bool showDebug = false;
	const Font debugFont{ 14, Typeface::Bold };

	while (System::Update())
	{
		const double deltaTime = Scene::DeltaTime();

		// ============================================================
		// 0. やり直し・デバッグ切り替え
		// ============================================================
		if (KeyR.down())
		{
			// 状態を最初に戻す（新しいオブジェクトを作って代入する）
			player = Tank{ map.getPlayerStart(), playerColor, Team::Player };
			enemies = SpawnEnemies(map);
			bullets.clear();
		}

		if (KeyF2.down())
		{
			showDebug = (not showDebug);
		}

		// ============================================================
		// 1. 更新
		// ============================================================
		// --- 戦車（撃った弾は bullets に追加される） ---
		if (player.isAlive())
		{
			player.update(MakePlayerInput(), deltaTime, map, bullets);
		}

		for (auto& enemy : enemies)
		{
			// -> は「ポインタが指す先のメンバ」を使う演算子。enemy.ai は unique_ptr（ポインタ）なので -> を使う。
			// ai がどの派生クラスでも、同じ書き方で think() を呼べる（virtual 関数の効果）。
			const TankInput input = enemy.ai->think(enemy.tank, player, map, deltaTime);
			enemy.tank.update(input, deltaTime, map, bullets);
		}

		// --- 弾 ---
		for (auto& bullet : bullets)
		{
			bullet.update(deltaTime, map);
		}

		// --- 弾と戦車の当たり判定 ---
		for (auto& bullet : bullets)
		{
			if (bullet.getTeam() == Team::Enemy)
			{
				// 敵の弾 → プレイヤーに当たるか
				if (player.isAlive() && player.getCircle().intersects(bullet.getCircle()))
				{
					player.destroy();
					bullet.destroy();
				}
			}
			else
			{
				// プレイヤーの弾 → どれかの敵に当たるか
				for (auto& enemy : enemies)
				{
					if (enemy.tank.isAlive() && enemy.tank.getCircle().intersects(bullet.getCircle()))
					{
						enemy.tank.destroy();
						bullet.destroy();
						break; // 1発の弾が2台を同時に撃破しないように、当たったらループを抜ける
					}
				}
			}
		}

		// --- 消えるものを取り除く ---
		bullets.remove_if([](const Bullet& b) { return (not b.isAlive()); });
		enemies.remove_if([](const Enemy& e) { return (not e.tank.isAlive()); });

		// ============================================================
		// 2. 描画
		// ============================================================
		map.draw();

		for (const auto& enemy : enemies)
		{
			enemy.tank.draw();
		}

		if (player.isAlive())
		{
			player.draw();
		}

		for (const auto& bullet : bullets)
		{
			bullet.draw();
		}

		// --- AI のデバッグ表示（F2） ---
		if (showDebug)
		{
			for (const auto& enemy : enemies)
			{
				const Vec2 pos = enemy.tank.getPos();

				// プレイヤーが見えていれば、視線を線で描く
				if (player.isAlive() && map.canSee(pos, player.getPos()))
				{
					Line{ pos, player.getPos() }.draw(1.0, ColorF{ 1.0, 0.0, 0.0, 0.3 });
				}

				debugFont(enemy.ai->getStateName()).drawAt(pos.movedBy(0, -32), ColorF{ 0.1 });
			}
		}

		// --- 状況表示（Stage 6 で HUD に置き換える予定） ---
		ClearPrint();
		Print << U"敵の残り: " << enemies.size();

		if (not player.isAlive())
		{
			Print << U"撃破された… R でやり直し";
		}
		else if (enemies.isEmpty())
		{
			Print << U"クリア！ R でもう一度";
		}
	}
}
