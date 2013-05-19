#pragma once
#include <Cheats.hh>

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem addr, value, saved;
	uint idx = 0;
	MenuItem *item[5] {nullptr};
	char addrStr[7] {0}, valueStr[3] {0}, savedStr[3] {0};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView();
	void init(bool highlightFirst, int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addCode;
	TextMenuItem cheat[EmuCheats::MAX];

	void loadAddCheatItems(MenuItem *item[], uint &items) override;
	void loadCheatItems(MenuItem *item[], uint &items) override;

public:
	EditCheatListView();
};

class CheatsView : public BaseCheatsView
{
private:
	BoolMenuItem cheat[EmuCheats::MAX];

	void loadCheatItems(MenuItem *item[], uint &i) override;

public:
	CheatsView() {}
};

extern CheatsView cheatsMenu;
