# pragma once
# include "Common.h"

// ------------------------------------------------------------
// リザルト画面（全ステージクリア / ゲームオーバー）
// ------------------------------------------------------------
class Result : public App::Scene
{
public:
	Result(const InitData& init);

	void update() override;

	void draw() const override;
};
