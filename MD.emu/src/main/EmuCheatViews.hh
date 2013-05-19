#pragma once
#include <Cheats.hh>
#include <main/Cheats.hh>
#include "system.h"
uint decodeCheat(const char *string, uint32 &address, uint16 &data, uint16 &originalData);

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem code;
	MdCheat *cheat = nullptr;
	MenuItem *item[3] = {nullptr};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView();
	void init(bool highlightFirst, MdCheat &cheat);
};

extern SystemEditCheatView editCheatView;

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
