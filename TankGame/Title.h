# pragma once
# include "Common.h"

// ------------------------------------------------------------
// タイトル画面
// ------------------------------------------------------------
// App::Scene（= IScene<String, GameData>）を継承して作る。
// Siv3D のシーンマネージャーが、update() と draw() を毎フレーム呼んでくれる。
class Title : public App::Scene
{
public:
	// シーンのコンストラクタは InitData を受け取り、基底クラスに渡す決まりになっている
	Title(const InitData& init);

	void update() override;

	void draw() const override;
};
