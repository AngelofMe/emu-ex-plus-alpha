#pragma once
#include "OptionView.hh"

static const char *installFirmwareFilesMessage =
	#if defined(CONFIG_BASE_ANDROID)
		"Install the C-BIOS BlueMSX machine files to your storage device?";
	#elif defined(CONFIG_ENV_WEBOS)
		"Install the C-BIOS BlueMSX machine files to internal storage? If using WebOS 1.4.5, make sure you have a version without the write permission bug.";
	#elif defined(CONFIG_BASE_IOS_JB)
		"Install the C-BIOS BlueMSX machine files to /User/Media/MSX.emu?";
	#else
		"Install the C-BIOS BlueMSX machine files to Machines directory?";
	#endif

static void installFirmwareFiles()
{
	CallResult ret = FsSys::mkdir(machineBasePath);
	if(ret != OK && ret != ALREADY_EXISTS)
	{
		popup.printf(4, 1, "Can't create directory:\n%s", machineBasePath);
		return;
	}

	const char *dirsToCreate[] =
	{
		"Machines", "Machines/MSX - C-BIOS",
		"Machines/MSX2 - C-BIOS", "Machines/MSX2+ - C-BIOS"
	};

	forEachDInArray(dirsToCreate, e)
	{
		FsSys::cPath pathTemp;
		snprintf(pathTemp, sizeof(pathTemp), "%s/%s", machineBasePath, e);
		CallResult ret = FsSys::mkdir(pathTemp);
		if(ret != OK && ret != ALREADY_EXISTS)
		{
			popup.printf(4, 1, "Can't create directory:\n%s", pathTemp);
			return;
		}
	}

	const char *srcPath[] =
	{
		"cbios.txt", "cbios.txt", "cbios.txt",
		"cbios_logo_msx1.rom", "cbios_main_msx1.rom", "config1.ini",
		"cbios_logo_msx2.rom", "cbios_main_msx2.rom", "cbios_sub.rom", "config2.ini",
		"cbios_logo_msx2+.rom", "cbios_main_msx2+.rom", "cbios_sub.rom", "cbios_music.rom", "config3.ini"
	};
	const char *destDir[] =
	{
			"MSX - C-BIOS", "MSX2 - C-BIOS", "MSX2+ - C-BIOS",
			"MSX - C-BIOS", "MSX - C-BIOS", "MSX - C-BIOS",
			"MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS",
			"MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS"
	};

	forEachDInArray(srcPath, e)
	{
		Io *src = openAppAssetIo(e);
		if(!src)
		{
			popup.printf(4, 1, "Can't open source file:\n %s", e);
			return;
		}
		FsSys::cPath pathTemp;
		snprintf(pathTemp, sizeof(pathTemp), "%s/Machines/%s/%s",
				machineBasePath, destDir[e_i], strstr(e, "config") ? "config.ini" : e);
		CallResult ret = copyIoToPath(src, pathTemp);
		delete src;
		if(ret != OK)
		{
			popup.printf(4, 1, "Can't write file:\n%s", e);
			return;
		}
	}

	string_copy(optionMachineNameStr, "MSX2 - C-BIOS", sizeof(optionMachineNameStr));
	popup.post("Installation OK");
}

class MsxMachineChoiceView : public BaseMultiChoiceView
{
public:
	constexpr MsxMachineChoiceView() { }
	MultiChoiceMenuItem *srcEntry = nullptr;
	TextMenuItem choiceEntry[256];
	MenuItem *choiceEntryItem[256] {nullptr};

	void init(MultiChoiceMenuItem *src, bool highlightFirst)
	{
		assert((uint)src->choices <= sizeofArray(choiceEntry));
		iterateTimes(src->choices, i)
		{
			choiceEntry[i].init(src->choiceStr[i], src->t2.face);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, src->choices, highlightFirst, C2DO);
		srcEntry = src;
	}

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		assert((int)i < srcEntry->choices);
		logMsg("set choice %d", i);
		srcEntry->setVal(i);
		removeModalView();
	}
};

static MsxMachineChoiceView msxMachineChoiceView;

class SystemOptionView : public OptionView
{
private:

	struct MsxMachineItem : public MultiChoiceSelectMenuItem
	{
		constexpr MsxMachineItem() { }

		static int dirFsFilter(const char *name, int type)
		{
			return type == Fs::TYPE_DIR;
		}

		uint machines = 0;
		char *machineName[256] {nullptr};

		void init()
		{
			FsSys f;
			static const char *title = "Machine Type";
			static const char *noneStr[] = { "None" };
			FsSys::cPath machinePath;
			snprintf(machinePath, sizeof(machinePath), "%s/Machines", machineBasePath);
			if(f.openDir(machinePath, 0, dirFsFilter) != OK)
			{
				logMsg("couldn't open %s", machinePath);
				MultiChoiceSelectMenuItem::init(title, noneStr, 0, 1);
				return;
			}

			int currentMachineIdx = -1;
			machines = 0;
			iterateTimes(std::min(f.numEntries(), 256U), i)
			{
				FsSys::cPath configPath;
				snprintf(configPath, sizeof(configPath), "%s/%s/config.ini", machinePath, f.entryFilename(i));
				if(!FsSys::fileExists(configPath))
				{
					logMsg("%s doesn't exist", configPath);
					continue;
				}
				machineName[machines] = string_dup(f.entryFilename(i));
				logMsg("added machine %s", f.entryFilename(i));
				if(string_equal(machineName[machines], optionMachineName))
				{
					logMsg("current machine is idx %d", i);
					currentMachineIdx = machines;
				}
				machines++;
			}

			if(!machines)
			{
				MultiChoiceSelectMenuItem::init(title, noneStr, 0, 1);
				return;
			}

			MultiChoiceSelectMenuItem::init(title, (const char **)machineName,
					currentMachineIdx, machines, 0, 1, currentMachineIdx == -1 ? "None" : 0);
		}

		void select(View *view, const Input::Event &e)
		{
			if(!machines)
			{
				popup.printf(4, 1, "Place machine directory in:\n%s", machineBasePath);
				return;
			}
			msxMachineChoiceView.init(this, !e.isPointer());
			msxMachineChoiceView.placeRect(Gfx::viewportRect());
			modalView = &msxMachineChoiceView;
		}

		void deinit()
		{
			logMsg("deinit MsxMachineItem");
			DualTextMenuItem::deinit();
			iterateTimes(machines, i)
			{
				mem_free(machineName[i]);
			}
		}

		void doSet(int val)
		{
			assert((uint)val < machines);
			string_copy(optionMachineName, machineName[val], sizeof(optionMachineName));
			logMsg("set machine type: %s", (char*)optionMachineName);
		}
	} msxMachine;

	#if !defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_IOS_JB)
	TextMenuItem installCBIOS
	{
		"Install MSX C-BIOS",
		[](TextMenuItem &, const Input::Event &e)
		{
			auto &ynAlertView = *allocModalView<YesNoAlertView>();
			ynAlertView.init(installFirmwareFilesMessage, !e.isPointer());
			ynAlertView.onYes() =
				[](const Input::Event &e)
				{
					installFirmwareFiles();
				};
			View::addModalView(ynAlertView);
		}
	};
	#endif

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward disk IO",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionSkipFdcAccess = item.on;
		}
	};

public:
	SystemOptionView() { }

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		OptionView::loadSystemItems(item, items);
		msxMachine.init(); item[items++] = &msxMachine;
		skipFdcAccess.init(optionSkipFdcAccess); item[items++] = &skipFdcAccess;
		#if !defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_IOS_JB)
		installCBIOS.init(); item[items++] = &installCBIOS;
		#endif
	}

	void init(uint idx, bool highlightFirst)
	{
		uint i = 0;
		switch(idx)
		{
			bcase 0: loadVideoItems(item, i);
			bcase 1: loadAudioItems(item, i);
			bcase 2: loadInputItems(item, i);
			bcase 3: loadSystemItems(item, i);
			bcase 4: loadGUIItems(item, i);
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

#include "MenuView.hh"

class MsxMediaFilePicker
{
public:
	enum { ROM, DISK, TAPE };

	static FsDirFilterFunc fsFilter(uint type)
	{
		FsDirFilterFunc filter = isMSXROMExtension;
		if(type == DISK)
			filter = isMSXDiskExtension;
		else if(type == TAPE)
			filter = isMSXTapeExtension;
		return filter;
	}
};

static const char *insertEjectDiskMenuStr[] { "Insert File", "Eject" };

static const char *insertEjectRomMenuStr[] { "Insert File", "Eject", "Insert SCC", "Insert SCC+", "Insert Sunrise IDE" };

class MsxIOControlView : public BaseMenuView
{
public:

	static const char *hdSlotPrefix[4];
	char hdSlotStr[4][1024] { {0} };

	void updateHDText(int slot)
	{
		FsSys::cPath basenameTemp;
		string_printf(hdSlotStr[slot], "%s %s", hdSlotPrefix[slot],
			strlen(hdName[slot]) ? string_basename(hdName[slot], basenameTemp) : "");
	}

	void updateHDStatusFromCartSlot(int cartSlot)
	{
		int hdSlotStart = cartSlot == 0 ? 0 : 2;
		hdSlot[hdSlotStart].active = hdSlot[hdSlotStart+1].active = boardGetHdType(cartSlot) == HD_SUNRISEIDE;
		updateHDText(hdSlotStart);
		updateHDText(hdSlotStart+1);
	}

	void onHDMediaChange(const char *name, int slot)
	{
		strcpy(hdName[slot], name);
		updateHDText(slot);
		hdSlot[slot].compile();
	}

	void addHDFilePickerView(const Input::Event &e, uint8 slot)
	{
		auto &fPicker = *allocModalView<EmuFilePicker>();
		fPicker.init(!e.isPointer(), MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK), 1);
		fPicker.onSelectFile() =
			[this, slot](const char* name, const Input::Event &e)
			{
				auto id = diskGetHdDriveId(slot / 2, slot % 2);
				logMsg("inserting hard drive id %d", id);
				if(insertDisk(name, id))
				{
					onHDMediaChange(name, slot);
				}
				View::removeModalView();
			};
		fPicker.onClose() = FSPicker::onCloseModalDefault();
		View::addModalView(fPicker);
	}

	void onSelectHD(TextMenuItem &item, const Input::Event &e, uint8 slot)
	{
		if(!item.active) return;
		if(strlen(hdName[slot]))
		{
			auto &multiChoiceView = *allocModalView<MultiChoiceView>();
			multiChoiceView.init(insertEjectDiskMenuStr, sizeofArray(insertEjectDiskMenuStr), !e.isPointer());
			multiChoiceView.onSelect() =
				[this, slot](int action, const Input::Event &e)
				{
					if(action == 0)
					{
						removeModalView();
						addHDFilePickerView(e, slot);
						Base::displayNeedsUpdate();
					}
					else
					{
						diskChange(diskGetHdDriveId(slot / 2, slot % 2), 0, 0);
						onHDMediaChange("", slot);
						removeModalView();
					}
					return 0;
				};
			View::addModalView(multiChoiceView);
		}
		else
		{
			addHDFilePickerView(e, slot);
			Base::displayNeedsUpdate();
		}
	}

	TextMenuItem hdSlot[4]
	{
		{[this](TextMenuItem &item, const Input::Event &e) { onSelectHD(item, e, 0); }},
		{[this](TextMenuItem &item, const Input::Event &e) { onSelectHD(item, e, 1); }},
		{[this](TextMenuItem &item, const Input::Event &e) { onSelectHD(item, e, 2); }},
		{[this](TextMenuItem &item, const Input::Event &e) { onSelectHD(item, e, 3); }}
	};

	static const char *romSlotPrefix[2];
	char romSlotStr[2][1024] { {0} };

	void updateROMText(int slot)
	{
		FsSys::cPath basenameTemp;
		string_printf(romSlotStr[slot], "%s %s", romSlotPrefix[slot],
			strlen(cartName[slot]) ? string_basename(cartName[slot], basenameTemp) : "");
	}

	void onROMMediaChange(const char *name, int slot)
	{
		strcpy(cartName[slot], name);
		updateROMText(slot);
		romSlot[slot].compile();
		updateHDStatusFromCartSlot(slot);
	}

	void addROMFilePickerView(const Input::Event &e, uint8 slot)
	{
		auto &fPicker = *allocModalView<EmuFilePicker>();
		fPicker.init(!e.isPointer(), MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::ROM), 1);
		fPicker.onSelectFile() =
			[this, slot](const char* name, const Input::Event &e)
			{
				if(insertROM(name, slot))
				{
					onROMMediaChange(name, slot);
				}
				View::removeModalView();
			};
		fPicker.onClose() = FSPicker::onCloseModalDefault();
		View::addModalView(fPicker);
	}

	void onSelectROM(const Input::Event &e, uint8 slot)
	{
		auto &multiChoiceView = *allocModalView<MultiChoiceView>();
		multiChoiceView.init(insertEjectRomMenuStr, sizeofArray(insertEjectRomMenuStr), !e.isPointer());
		multiChoiceView.onSelect() =
			[this, slot](int action, const Input::Event &e)
			{
				if(action == 0)
				{
					removeModalView();
					addROMFilePickerView(e, slot);
					Base::displayNeedsUpdate();
				}
				else if(action == 1)
				{
					boardChangeCartridge(slot, ROM_UNKNOWN, 0, 0);
					onROMMediaChange("", slot);
					removeModalView();
				}
				else if(action == 2)
				{
					boardChangeCartridge(slot, ROM_SCC, "", 0);
					onROMMediaChange("SCC", slot);
					removeModalView();
				}
				else if(action == 3)
				{
					boardChangeCartridge(slot, ROM_SCCPLUS, "", 0);
					onROMMediaChange("SCC+", slot);
					removeModalView();
				}
				else if(action == 4)
				{
					if(!boardChangeCartridge(slot, ROM_SUNRISEIDE, "Sunrise IDE", 0))
					{
						popup.postError("Error loading Sunrise IDE device");
					}
					else
						onROMMediaChange("Sunrise IDE", slot);
					removeModalView();
				}
				return 0;
			};
		View::addModalView(multiChoiceView);
	}

	TextMenuItem romSlot[2]
	{
		{[this](TextMenuItem &, const Input::Event &e) { onSelectROM(e, 0); }},
		{[this](TextMenuItem &, const Input::Event &e) { onSelectROM(e, 1); }}
	};

	static const char *diskSlotPrefix[2];
	char diskSlotStr[2][1024] { {0} };

	void updateDiskText(int slot)
	{
		FsSys::cPath basenameTemp;
		string_printf(diskSlotStr[slot], "%s %s", diskSlotPrefix[slot],
				strlen(diskName[slot]) ? string_basename(diskName[slot], basenameTemp) : "");
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		strcpy(diskName[slot], name);
		updateDiskText(slot);
		diskSlot[slot].compile();
	}

	void addDiskFilePickerView(const Input::Event &e, uint8 slot)
	{
		auto &fPicker = *allocModalView<EmuFilePicker>();
		fPicker.init(!e.isPointer(), MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK), 1);
		fPicker.onSelectFile() =
			[this, slot](const char* name, const Input::Event &e)
			{
				logMsg("inserting disk in slot %d", slot);
				if(insertDisk(name, slot))
				{
					onDiskMediaChange(name, slot);
				}
				View::removeModalView();
			};
		fPicker.onClose() = FSPicker::onCloseModalDefault();
		View::addModalView(fPicker);
	}

	void onSelectDisk(const Input::Event &e, uint8 slot)
	{
		if(strlen(diskName[slot]))
		{
			auto &multiChoiceView = *allocModalView<MultiChoiceView>();
			multiChoiceView.init(insertEjectDiskMenuStr, sizeofArray(insertEjectDiskMenuStr), !e.isPointer());
			multiChoiceView.onSelect() =
				[this, slot](int action, const Input::Event &e)
				{
					if(action == 0)
					{
						removeModalView();
						addDiskFilePickerView(e, slot);
						Base::displayNeedsUpdate();
					}
					else
					{
						diskChange(slot, 0, 0);
						onDiskMediaChange("", slot);
						removeModalView();
					}
					return 0;
				};
			View::addModalView(multiChoiceView);
		}
		else
		{
			addDiskFilePickerView(e, slot);
		}
		Base::displayNeedsUpdate();
	}

	TextMenuItem diskSlot[2]
	{
		{[this](TextMenuItem &, const Input::Event &e) { onSelectDisk(e, 0); }},
		{[this](TextMenuItem &, const Input::Event &e) { onSelectDisk(e, 1); }}
	};

	MenuItem *item[9] {nullptr};
public:
	MsxIOControlView(): BaseMenuView("IO Control") { }

	void init(bool highlightFirst)
	{
		uint i = 0;
		iterateTimes(2, slot)
		{
			updateROMText(slot);
			romSlot[slot].init(romSlotStr[slot], (int)slot < boardInfo.cartridgeCount); item[i++] = &romSlot[slot];
		}
		iterateTimes(2, slot)
		{
			updateDiskText(slot);
			diskSlot[slot].init(diskSlotStr[slot], (int)slot < boardInfo.diskdriveCount); item[i++] = &diskSlot[slot];
		}
		iterateTimes(4, slot)
		{
			updateHDText(slot);
			hdSlot[slot].init(hdSlotStr[slot], boardGetHdType(slot/2) == HD_SUNRISEIDE); item[i++] = &hdSlot[slot];
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static MsxIOControlView msxIoMenu;

const char *MsxIOControlView::romSlotPrefix[2] {"ROM1:", "ROM2:"};
const char *MsxIOControlView::diskSlotPrefix[2] {"Disk1:", "Disk2:"};
const char *MsxIOControlView::hdSlotPrefix[4] {"IDE1-M:", "IDE1-S:", "IDE2-M:", "IDE2-S:"};

class SystemMenuView : public MenuView
{
private:
	TextMenuItem msxIOControl
	{
		"ROM/Disk Control",
		[](TextMenuItem &item, const Input::Event &e)
		{
			if(item.active)
			{
				FsSys::chdir(EmuSystem::gamePath);// Stay in active media's directory
				msxIoMenu.init(!e.isPointer());
				viewStack.pushAndShow(&msxIoMenu);
			}
			else if(EmuSystem::gameIsRunning() && activeBoardType != BOARD_MSX)
			{
				popup.post("Only used in MSX mode", 2);
			}
		}
	};

public:
	SystemMenuView() { }

	void gameStopped()
	{
		msxIOControl.active = 0;
		reset.active = 0;
		loadState.active = 0;
		saveState.active = 0;
	}

	void onShow()
	{
		MenuView::onShow();
		msxIOControl.active = EmuSystem::gameIsRunning() && activeBoardType == BOARD_MSX;
	}

	void init(bool highlightFirst)
	{
		uint items = 0;
		loadFileBrowserItems(item, items);
		msxIOControl.init(); item[items++] = &msxIOControl;
		loadStandardItems(item, items);
		assert(items <= sizeofArray(item));
		BaseMenuView::init(item, items, highlightFirst);
	}
};
