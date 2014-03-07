/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <meta.h>
#include <MenuView.hh>
#include <Recent.hh>
#include <gui/AlertView.hh>
#include <EmuApp.hh>
#include <EmuSystem.hh>
#include <CreditsView.hh>
#include <FilePicker.hh>
#include <StateSlotView.hh>
#include <EmuInput.hh>
#include <EmuOptions.hh>
#include <InputManagerView.hh>
#include <TouchConfigView.hh>
#include <BundledGamesView.hh>
#include <TextEntry.hh>
#include <util/strings.h>
#ifdef CONFIG_BLUETOOTH
#include <bluetooth/sys.hh>
#include <bluetooth/BluetoothInputDevScanner.hh>
#endif
extern const char *creditsViewStr;
#ifdef CONFIG_BLUETOOTH
extern BluetoothAdapter *bta;
#endif

char saveSlotChar(int slot)
{
	switch(slot)
	{
		case -1: return 'a';
		case 0 ... 9: return 48 + slot;
		default: bug_branch("%d", slot); return 0;
	}
}

#ifdef CONFIG_BLUETOOTH

static bool initBTAdapter()
{
	if(bta)
	{
		return true;
	}

	logMsg("initializing Bluetooth");
	bta = BluetoothAdapter::defaultAdapter();
	return bta;
}

static auto onScanStatus =
	[](BluetoothAdapter &, uint status, int arg)
	{
		switch(status)
		{
			bcase BluetoothAdapter::INIT_FAILED:
			{
				if(Config::envIsIOS)
				{
					popup.postError("BTstack power on failed, make sure the iOS Bluetooth stack is not active");
					Base::mainWindow().postDraw();
				}
			}
			bcase BluetoothAdapter::SCAN_FAILED:
			{
				popup.postError("Scan failed");
				Base::mainWindow().postDraw();
			}
			bcase BluetoothAdapter::SCAN_NO_DEVS:
			{
				popup.post("No devices found");
				Base::mainWindow().postDraw();
			}
			bcase BluetoothAdapter::SCAN_PROCESSING:
			{
				popup.printf(2, 0, "Checking %d device(s)...", arg);
				Base::mainWindow().postDraw();
			}
			bcase BluetoothAdapter::SCAN_NAME_FAILED:
			{
				popup.postError("Failed reading a device name");
				Base::mainWindow().postDraw();
			}
			bcase BluetoothAdapter::SCAN_COMPLETE:
			{
				int devs = Bluetooth::pendingDevs();
				if(devs)
				{
					popup.printf(2, 0, "Connecting to %d device(s)...", devs);
					Bluetooth::connectPendingDevs(bta);
				}
				else
				{
					popup.post("Scan complete, no recognized devices");
				}
				Base::mainWindow().postDraw();
			}
			/*bcase BluetoothAdapter::SOCKET_OPEN_FAILED:
			{
				popup.postError("Failed opening a Bluetooth connection");
				Base::mainWindow().postDraw();
			}*/
		}
	};

static void handledFailedBTAdapterInit(const Input::Event &e)
{
	popup.postError("Unable to initialize Bluetooth adapter");
	#ifdef CONFIG_BLUETOOTH_BTSTACK
	if(!FsSys::fileExists("/var/lib/dpkg/info/ch.ringwald.btstack.list"))
	{
		auto &ynAlertView = *allocModalView<YesNoAlertView>(Base::mainWindow());
		ynAlertView.init("BTstack not found, open Cydia and install?", !e.isPointer());
		ynAlertView.onYes() =
			[](const Input::Event &e)
			{
				logMsg("launching Cydia");
				Base::openURL("cydia://package/ch.ringwald.btstack");
			};
		modalViewController.pushAndShow(ynAlertView);
	}
	#endif
}

#endif

void MenuView::onShow()
{
	logMsg("refreshing main menu state");
	recentGames.active = recentGameList.size();
	reset.active = EmuSystem::gameIsRunning();
	saveState.active = EmuSystem::gameIsRunning();
	loadState.active = EmuSystem::gameIsRunning() && EmuSystem::stateExists(EmuSystem::saveStateSlot);
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.compile();
	screenshot.active = EmuSystem::gameIsRunning();
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon.active = EmuSystem::gameIsRunning();
	#endif
	#ifdef CONFIG_BLUETOOTH
	bluetoothDisconnect.active = Bluetooth::devsConnected();
	#endif
}

void MenuView::loadFileBrowserItems(MenuItem *item[], uint &items)
{
	loadGame.init(); item[items++] = &loadGame;
	recentGames.init(); item[items++] = &recentGames;
	#ifdef EMU_FRAMEWORK_BUNDLED_GAMES
	if(optionShowBundledGames)
	{
		bundledGames.init(); item[items++] = &bundledGames;
	}
	#endif
}

void MenuView::loadStandardItems(MenuItem *item[], uint &items)
{
	reset.init(); item[items++] = &reset;
	loadState.init(); item[items++] = &loadState;
	saveState.init(); item[items++] = &saveState;
	stateSlotText[12] = saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.init(stateSlotText); item[items++] = &stateSlot;
	if(!Config::MACHINE_IS_OUYA)
	{
		onScreenInputManager.init(); item[items++] = &onScreenInputManager;
	}
	inputManager.init(); item[items++] = &inputManager;
	options.init(); item[items++] = &options;
	#ifdef CONFIG_BLUETOOTH
	scanWiimotes.init(); item[items++] = &scanWiimotes;
		#ifdef CONFIG_BLUETOOTH_SERVER
		acceptPS3ControllerConnection.init(); item[items++] = &acceptPS3ControllerConnection;
		#endif
	bluetoothDisconnect.init(); item[items++] = &bluetoothDisconnect;
	#endif
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon.init(); item[items++] = &addLauncherIcon;
	#endif
	benchmark.init(); item[items++] = &benchmark;
	screenshot.init(); item[items++] = &screenshot;
	about.init(); item[items++] = &about;
	exitApp.init(); item[items++] = &exitApp;
}

MenuView::MenuView(Base::Window &win):
	BaseMenuView(CONFIG_APP_NAME " " IMAGINE_VERSION, win),
	loadGame
	{
		"Load Game",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &fPicker = *menuAllocator.allocNew<EmuFilePicker>(window());
			fPicker.init(!e.isPointer(), false);
			pushAndShow(fPicker, &menuAllocator, false);
		}
	},
	reset
	{
		"Reset",
		[this](TextMenuItem &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
				ynAlertView.init("Really Reset Game?", !e.isPointer());
				ynAlertView.onYes() =
					[](const Input::Event &e)
					{
						EmuSystem::resetGame();
						startGameFromMenu();
					};
				modalViewController.pushAndShow(ynAlertView);
			}
		}
	},
	loadState
	{
		"Load State",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(item.active && EmuSystem::gameIsRunning())
			{
				auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
				ynAlertView.init("Really Load State?", !e.isPointer());
				ynAlertView.onYes() =
					[](const Input::Event &e)
					{
						int ret = EmuSystem::loadState();
						if(ret != STATE_RESULT_OK)
						{
							if(ret != STATE_RESULT_OTHER_ERROR) // check if we're responsible for posting the error
								popup.postError(stateResultToStr(ret));
						}
						else
							startGameFromMenu();
					};
				modalViewController.pushAndShow(ynAlertView);
			}
		}
	},
	recentGames
	{
		"Recent Games",
		[this](TextMenuItem &, const Input::Event &e)
		{
			if(recentGameList.size())
			{
				auto &rMenu = *menuAllocator.allocNew<RecentGameView>(window());
				rMenu.init(!e.isPointer());
				pushAndShow(rMenu, &menuAllocator);
			}
		}
	},
	#ifdef EMU_FRAMEWORK_BUNDLED_GAMES
	bundledGames
	{
		"Bundled Games",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &bMenu = *menuAllocator.allocNew<BundledGamesView>(window());
			bMenu.init(!e.isPointer());
			pushAndShow(bMenu, &menuAllocator);
		}
	},
	#endif
	saveState
	{
		"Save State",
		[this](TextMenuItem &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				static auto doSaveState =
					[]()
					{
						int ret = EmuSystem::saveState();
						if(ret != STATE_RESULT_OK)
							popup.postError(stateResultToStr(ret));
						else
							startGameFromMenu();
					};

				if(EmuSystem::shouldOverwriteExistingState())
				{
					doSaveState();
				}
				else
				{
					auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
					ynAlertView.init("Really Overwrite State?", !e.isPointer());
					ynAlertView.onYes() =
						[](const Input::Event &e)
						{
							doSaveState();
						};
					modalViewController.pushAndShow(ynAlertView);
				}
			}
		}
	},
	stateSlot
	{
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &ssMenu = *menuAllocator.allocNew<StateSlotView>(window());
			ssMenu.init(!e.isPointer());
			pushAndShow(ssMenu, &menuAllocator);
		}
	},
	stateSlotText // Can't init with string literal due to GCC bug #43453
		{'S', 't', 'a', 't', 'e', ' ', 'S', 'l', 'o', 't', ' ', '(', '0', ')' },
	options
	{
		"Options",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &oMenu = *menuAllocator.allocNew<OptionCategoryView>(window());
			oMenu.init(!e.isPointer());
			pushAndShow(oMenu, &menuAllocator);
		}
	},
	onScreenInputManager
	{
		"On-screen Input Setup",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &tcMenu = *menuAllocator.allocNew<TouchConfigView>(window(), touchConfigFaceBtnName, touchConfigCenterBtnName);
			tcMenu.init(!e.isPointer());
			pushAndShow(tcMenu, &menuAllocator);
		}
	},
	inputManager
	{
		"Key/Gamepad Input Setup",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &menu = *menuAllocator.allocNew<InputManagerView>(window());
			menu.init(!e.isPointer());
			pushAndShow(menu, &menuAllocator);
		}
	},
	benchmark
	{
		"Benchmark Game",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &fPicker = *allocModalView<EmuFilePicker>(window());
			fPicker.initForBenchmark(!e.isPointer());
			modalViewController.pushAndShow(fPicker);
		}
	},
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon
	{
		"Add Game Shortcut to Launcher",
		[this](TextMenuItem &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(!strlen(EmuSystem::gamePath()))
				{
					// shortcuts to bundled games not yet supported
					return;
				}
				auto &textInputView = *allocModalView<CollectTextInputView>(window());
				textInputView.init("Shortcut Name", EmuSystem::fullGameName(),
						getCollectTextCloseAsset());
				textInputView.onText() =
					[this](CollectTextInputView &view, const char *str)
					{
						if(str && strlen(str))
						{
							Base::addLauncherIcon(str, EmuSystem::fullGamePath());
							popup.printf(2, false, "Added shortcut:\n%s", str);
							postDraw();
						}
						view.dismiss();
						return 0;
					};
				modalViewController.pushAndShow(textInputView);
			}
			else
			{
				popup.post("Load a game first");
			}
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH
	scanWiimotes
	{
		"Scan for Wiimotes/iCP/JS1",
		[this](TextMenuItem &t, const Input::Event &e)
		{
			if(initBTAdapter())
			{
				if(Bluetooth::scanForDevices(*bta, onScanStatus))
				{
					popup.post("Starting Scan...\n(see website for device-specific help)", 4);
				}
				else
				{
					popup.post("Still scanning", 1);
				}
			}
			else
			{
				handledFailedBTAdapterInit(e);
			}
			postDraw();
		}
	},
	bluetoothDisconnect
	{
		"Disconnect Bluetooth",
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(Bluetooth::devsConnected())
			{
				static char str[64];
				snprintf(str, sizeof(str), "Really disconnect %d Bluetooth device(s)?", Bluetooth::devsConnected());
				auto &ynAlertView = *allocModalView<YesNoAlertView>(window());
				ynAlertView.init(str, !e.isPointer());
				ynAlertView.onYes() =
					[](const Input::Event &e)
					{
						Bluetooth::closeBT(bta);
					};
				modalViewController.pushAndShow(ynAlertView);
			}
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SERVER
	acceptPS3ControllerConnection
	{
		"Scan for PS3 Controller",
		[this](TextMenuItem &t, const Input::Event &e)
		{
			if(initBTAdapter())
			{
				popup.post("Prepare to push the PS button", 4);
				auto startedScan = Bluetooth::listenForDevices(*bta,
					[this](BluetoothAdapter &bta, uint status, int arg)
					{
						switch(status)
						{
							bcase BluetoothAdapter::INIT_FAILED:
							{
								popup.postError(Config::envIsLinux ? "Unable to register server, make sure this executable has cap_net_bind_service enabled and bluetoothd isn't running" :
									"Bluetooth setup failed", Config::envIsLinux ? 8 : 2);
								postDraw();
							}
							bcase BluetoothAdapter::SCAN_COMPLETE:
							{
								popup.post("Push the PS button on your controller\n(see website for pairing help)", 4);
								postDraw();
							}
							bdefault: onScanStatus(bta, status, arg);
						}
					});
				if(!startedScan)
				{
					popup.post("Still scanning", 1);
				}
			}
			else
			{
				handledFailedBTAdapterInit(e);
			}
			postDraw();
		}
	},
	#endif
	about
	{
		"About",
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto &credits = *menuAllocator.allocNew<CreditsView>(creditsViewStr, window());
			credits.init();
			pushAndShow(credits, &menuAllocator);
		}
	},
	exitApp
	{
		"Exit",
		[](TextMenuItem &, const Input::Event &e)
		{
			Base::exit();
		}
	},
	screenshot
	{
		"Game Screenshot",
		[](TextMenuItem &, const Input::Event &e)
		{
			if(EmuSystem::gameIsRunning())
				emuView.takeGameScreenshot();
		}
	}
{}

void OptionCategoryView::init(bool highlightFirst)
{
	//logMsg("running option category init");
	uint i = 0;
	forEachInArray(subConfig, e)
	{
		e->init(); item[i++] = e;
		e->onSelect() =
		[this, e_i](TextMenuItem &, const Input::Event &e)
		{
			auto &oCategoryMenu = allocAndGetOptionCategoryMenu(window(), e, menuAllocator, e_i);
			viewStack.pushAndShow(oCategoryMenu, &menuAllocator);
		};
	}
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

extern void loadGameComplete(bool tryAutoState, bool addToRecent);

static void loadGameCompleteConfirmYesAutoLoadState(const Input::Event &e)
{
	loadGameComplete(1, 0);
}

static void loadGameCompleteConfirmNoAutoLoadState(const Input::Event &e)
{
	loadGameComplete(0, 0);
}

bool showAutoStateConfirm(const Input::Event &e, bool addToRecent);

void loadGameCompleteFromRecentItem(uint result, const Input::Event &e)
{
	if(!result)
		return;

	if(!showAutoStateConfirm(e, false))
	{
		loadGameComplete(1, 0);
	}
}

void RecentGameInfo::handleMenuSelection(TextMenuItem &, const Input::Event &e)
{
	FsSys::cPath dirnameTemp;
	FsSys::chdir(string_dirname(path, dirnameTemp));
	EmuSystem::onLoadGameComplete() =
		[](uint result, const Input::Event &e)
		{
			loadGameCompleteFromRecentItem(result, e);
		};
	auto res = EmuSystem::loadGame(path);
	if(res == 1)
	{
		loadGameCompleteFromRecentItem(1, e);
	}
	else if(res == 0)
	{
		EmuSystem::clearGamePaths();
	}
}

void RecentGameView::init(bool highlightFirst)
{
	uint i = 0;
	int rIdx = 0;
	for(auto &e : recentGameList)
	{
		recentGame[rIdx].init(e.name, FsSys::fileExists(e.path)); item[i++] = &recentGame[rIdx];
		recentGame[rIdx].onSelect() = [&](TextMenuItem &t, const Input::Event &ev) {e.handleMenuSelection(t,ev);};
		rIdx++;
	}
	clear.init(recentGameList.size()); item[i++] = &clear;
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

RecentGameView::RecentGameView(Base::Window &win):
	BaseMenuView("Recent Games", win),
	clear
	{
		"Clear List",
		[this](TextMenuItem &t, const Input::Event &e)
		{
			recentGameList.clear();
			dismiss();
		}
	}
{}
