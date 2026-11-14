# pragma once
# include "Common.h"
# include "Tank.h"
# include "EnemyAI.h"
# include "Effects.h"

// ------------------------------------------------------------
// 敵 = 戦車（体）+ AI（頭脳）
// ------------------------------------------------------------
struct Enemy
{
	Tank tank;
	std::unique_ptr<EnemyAI> ai;
};

// ------------------------------------------------------------
// ゲーム画面（1ステージ分）
// ------------------------------------------------------------
// Stage 5 までは Main() の中のローカル変数だったもの（map, player, enemies, bullets）を、
// このクラスのメンバ変数として持つ。
// ステージをやり直すときや次のステージへ進むときは、シーンを作り直す（= 全部が初期状態に戻る）。
class Game : public App::Scene
{
public:
	Game(const InitData& init);

	void update() override;

	void draw() const override;

private:
	// ステージの進行段階
	enum class Phase
	{
		Ready,     // 開始前の「READY」表示。まだ誰も動かない
		Playing,   // プレイ中
		Cleared,   // 敵を全滅させた。少し待って次へ
		Destroyed, // プレイヤーが撃破された。少し待ってやり直し or ゲームオーバー
	};

	Map m_map;
	Tank m_player;
	Array<Enemy> m_enemies;
	Array<Bullet> m_bullets;

	Phase m_phase = Phase::Ready;
	double m_phaseTimer = 0.0;     // 今のフェーズに入ってからの時間 [秒]
	bool m_sceneChanging = false;  // changeScene() を呼んだか（2回呼ばないため）
	bool m_showDebug = false;      // AI のデバッグ表示

	// --- 演出 ---
	Effect m_effect;               // 爆発・閃光・火花をまとめて管理する
	double m_shakeTime = 0.0;      // 画面の揺れの残り時間 [秒]
	double m_shakePower = 0.0;     // 画面の揺れの強さ [ピクセル]
	Vec2 m_shakeOffset{ 0, 0 };    // このフレームの揺れのずれ（update で決めて draw で使う）

	// フェーズを切り替える（タイマーもリセットする）
	void setPhase(Phase phase);

	// 戦車が撃破されたときの演出（爆発・効果音・画面の揺れ）
	void onTankDestroyed(const Tank& tank);

	// 画面を揺らす（今より強い揺れなら上書きする）
	void startShake(double power, double time);

	// 戦車・弾を1フレーム進め、当たり判定をする。
	// playerControllable が false のときは、プレイヤーの戦車は動かさない。
	void updateWorld(double deltaTime, bool playerControllable);

	// Cleared / Destroyed のあと、次のシーンへ進む
	void goToNextScene();

	void drawHUD() const;
	void drawCenterMessage(const String& text, const ColorF& color) const;
};
