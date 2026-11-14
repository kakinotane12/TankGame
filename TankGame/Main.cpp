# include <Siv3D.hpp> // Siv3D v0.6.16
# include "Common.h"
# include "Title.h"
# include "Game.h"
# include "Result.h"

//
// Stage 7: 演出（爆発・発射の閃光・跳弾の火花・効果音・画面の揺れ）
//
// 操作:
//   W / S : 前進 / 後退
//   A / D : 車体を左回転 / 右回転
//   マウス : 砲塔の向き
//   左クリック : 発射
//   F2 : AI のデバッグ表示（状態と視線）の切り替え（F1 は Siv3D がライセンス表示に使っているので避ける）
//
// 全 3 ステージ。敵を全滅させると次のステージへ。撃破されると残機が減り、0 になるとゲームオーバー。
//
// Stage 5 まで Main() に書いていたゲームの処理は Game クラス（Game.h / Game.cpp）に移した。
// Main() の仕事は「準備をして、シーンマネージャーを回す」だけになる。
//

void Main()
{
	Window::SetTitle(U"TankGame");

	// ------------------------------------------------------------
	// フォントをアセットとして登録する
	// ------------------------------------------------------------
	// FontAsset に名前を付けて登録しておくと、どのシーンからでも FontAsset(U"名前") で使える。
	// Font を作るのは重い処理なので、シーンが切り替わるたびに作り直さずに済む。
	FontAsset::Register(U"Title", FontMethod::MSDF, 64, Typeface::Heavy); // MSDF: 拡大しても滲まない方式
	FontAsset::Register(U"Large", FontMethod::MSDF, 48, Typeface::Bold);
	FontAsset::Register(U"Medium", 22, Typeface::Bold);
	FontAsset::Register(U"Small", 14, Typeface::Bold);

	// ------------------------------------------------------------
	// 効果音をアセットとして登録する
	// ------------------------------------------------------------
	// 音声ファイルを用意しなくても、Siv3D に内蔵された楽器の音（GM 音源）から効果音を作れる。
	// AudioAsset::Register(名前, 楽器, 音の高さ, 長さ)
	AudioAsset::Register(U"Fire", GMInstrument::Gunshot, PianoKey::C4, 0.3s);
	AudioAsset::Register(U"Bounce", GMInstrument::Woodblock, PianoKey::C6, 0.1s);
	AudioAsset::Register(U"Explosion", GMInstrument::Timpani, PianoKey::C2, 0.8s);

	// ------------------------------------------------------------
	// シーンマネージャー
	// ------------------------------------------------------------
	App manager;

	// シーンを名前付きで登録する。最初に登録したシーンから始まる。
	// add<クラス>(名前) は manager 自身の参照を返すので、続けて .add と書ける（メソッドチェーン）。
	manager
		.add<Title>(U"Title")
		.add<Game>(U"Game")
		.add<Result>(U"Result");

	// シーン切り替え時のフェードの色
	manager.setFadeColor(ColorF{ 0.1 });

	while (System::Update())
	{
		// 今のシーンの update() と draw() が呼ばれる。エラーが起きたら false が返る。
		if (not manager.update())
		{
			break;
		}
	}
}
