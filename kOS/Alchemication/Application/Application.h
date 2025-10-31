/******************************************************************/
/*!
\file      Application.h
\author    Ng Jaz winn, jazwinn.ng , 2301502
\par       jazwinn.ng@digipen.edu
\date      Oct 02, 2024
\brief     This file contains the declaration to the main update 
		   loop of the engine. It initializes the various major 
		   systems and call upon them in the update loop.

Copyright (C) 2024 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/********************************************************************/
#ifndef APP_H
#define APP_H

#include "Debugging/Logging.h"
#include "ECS/ECS.h"
#include "Inputs/Input.h"
#include "Window.h"
#include "Reflection/Reflection.h"
/******************************************************************/
/*!
\class     Application
\brief     The Papplciation class is responsible for the entire engine.
		   It calls onto all the systems.
*/
/******************************************************************/
namespace Application {

	class Application {

		

	public:
		Application() :lvWindow() {}
		~Application() = default;

		int Init();
		int Run();
		int m_Cleanup();


	private:

		AppWindow lvWindow;
		logging::Logger logs;
	};

}


#endif APP_H


