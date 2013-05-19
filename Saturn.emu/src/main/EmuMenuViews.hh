#pragma once
#include "OptionView.hh"

class SystemOptionView : public OptionView
{
public:

	BiosSelectMenu biosSelectMenu {&::biosPath, ssBiosFsFilter};
	char biosPathStr[256] {0};
	TextMenuItem biosPath
	{
		"",
		[this](TextMenuItem &, const Input::Event &e)
		{
			biosSelectMenu.init(!e.isPointer());
			biosSelectMenu.placeRect(Gfx::viewportRect());
			biosSelectMenu.onBiosChange() =
				[this]()
				{
					logMsg("set bios %s", ::biosPath);
					printBiosMenuEntryStr(biosPathStr);
					biosPath.compile();
				};
			modalView = &biosSelectMenu;
			Base::displayNeedsUpdate();
		}
	};

	template <size_t S>
	static void printBiosMenuEntryStr(char (&str)[S])
	{
		FsSys::cPath basenameTemp;
		string_printf(str, "BIOS: %s", strlen(::biosPath) ? string_basename(::biosPath, basenameTemp) : "None set");
	}

	MultiChoiceSelectMenuItem sh2Core
	{
		"SH2",
		[](MultiChoiceMenuItem &, int val)
		{
			assert(val < (int)sizeofArray(SH2CoreList)-1);
			yinit.sh2coretype = SH2CoreList[val]->id;
		}
	};

	void sh2CoreInit()
	{
		static const char *str[6];

		int setting = 0, cores = 0;
		iterateTimes(sizeofArray(SH2CoreList)-1, i)
		{
			if(i == sizeofArray(str))
				break;
			str[i] = SH2CoreList[i]->Name;
			if(SH2CoreList[i]->id == yinit.sh2coretype)
				setting = i;
			cores++;
		}

		sh2Core.init(str, setting, cores);
	}

public:
	SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		if(sizeofArray(SH2CoreList) > 2)
		{
			sh2CoreInit(); item[items++] = &sh2Core;
		}
		printBiosMenuEntryStr(biosPathStr);
		biosPath.init(biosPathStr); item[items++] = &biosPath;
	}
};

#include "MenuView.hh"

class SystemMenuView : public MenuView
{
public:
	SystemMenuView() { }
};
