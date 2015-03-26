#include "UserInterface.h"
#include <cover/mui/coMUIToggleButton.h>
#include <cover/mui/coMUITab.h>
#include <cover/mui/coMUIPotiSlider.h>
#include <cover/mui/coMUIFrame.h>
#include <cover/mui/support/coMUIConfigManager.h>
#include <cover/mui/coMUILabel.h>
#include <cover/mui/coMUIContainer.h>
#include <iostream>

#include <cover/coTabletUI.h>
#include <cover/coVRTui.h>


UserInterface::UserInterface()
{
    ConfigManager=NULL;
}

UserInterface::~UserInterface(){
}

bool UserInterface::init()
{
    // get Instance of ConfigManager
    ConfigManager=coMUIConfigManager::getInstance();

    // create new Tab in TabFolder
    Tab1.reset(new coMUITab("plugins.examples.UserInterface.Tab1", "Tab1"));

    // create new ToggleButton in Tab1
    Button1.reset(new coMUIToggleButton("plugins.examples.UserInterface.Button1", Tab1.get(), "Button1"));
    Button1->setEventListener(this);
    Button1->setPos(0,0);                                                                                   // will be ignored, if there exists a positioning instruction in configuration file

    Button2.reset(new coMUIToggleButton("plugins.examples.UserInterface.Button2", Tab1.get(), "Button2"));
    Button2->setEventListener(this);
    Button2->setPos(1,0);                                                                                   // will be obeyed, if there is no positioning instruction for this element in the configuration file

    // create new PotiSlider in Tab1
    Slider1.reset(new coMUIPotiSlider("plugins.examples.UserInterface.Slider1", Tab1.get(), 0., 100., 50., "Slider1"));
    Slider1->setPos(0,1);                                                                                   // will be obeyed, if there is no positioning instruction for this element in the configuration file

    // create new Frame-Element
    Frame.reset(new coMUIFrame("plugins.examples.UserInterface.Frame", Tab1.get(), "Frame"));
    Frame->setPos(0,2);                                                                                     // will be obeyed, if there is no positioning instruction for this element in the configuration file

    // create new Label-Element
    Label.reset(new coMUILabel("plugins.examples.UserInterface.Label1", Frame.get(), "Label"));
    Label->setPos(0,3);                                                                                     // will be obeyed, if there is no positioning instruction for this element in the configuration file

    return true;
}

void UserInterface::muiEvent(coMUIElement *muiItem)
{
    if (muiItem == Button1.get())
    {
        Button2->click();
    }
    if (muiItem == Button2.get())
    {
        Button1->click();
    }
}

COVERPLUGIN(UserInterface)
