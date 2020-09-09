/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "C:\Users\nicho\Desktop\Uni work\Year 3\Nicholas Poole, 20714661, co3301, 16.01.20\new upload\new upload\CParseLevel.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "TankAssignment.h"

namespace gen
{

	//-----------------------------------------------------------------------------
	// Constants
	//-----------------------------------------------------------------------------

	// Control speed
	const float CameraRotSpeed = 2.0f;
	float CameraMoveSpeed = 80.0f;

	// Amount of time to pass before calculating new average update time
	const float UpdateTimePeriod = 1.0f;


	//-----------------------------------------------------------------------------
	// Global system variables
	//-----------------------------------------------------------------------------

	// Get reference to global DirectX variables from another source file
	extern ID3D10Device* g_pd3dDevice;
	extern IDXGISwapChain* SwapChain;
	extern ID3D10DepthStencilView* DepthStencilView;
	extern ID3D10RenderTargetView* BackBufferRenderTarget;
	extern ID3DX10Font* OSDFont;

	// Actual viewport dimensions (fullscreen or windowed)
	extern TUInt32 ViewportWidth;
	extern TUInt32 ViewportHeight;

	// Current mouse position
	extern TUInt32 MouseX;
	extern TUInt32 MouseY;

	extern CVector2 mousePixelVector;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;


	//-----------------------------------------------------------------------------
	// Global game/scene variables
	//-----------------------------------------------------------------------------

	// timer for dropping ammo crates

	extern TFloat32 AmmoDropTimer = 20.0f;
	extern bool AmmoDropClaimed = true;

	// Entity manager
	CEntityManager EntityManager;
	CParseLevel LevelParser(&EntityManager);

	// Tank UIDs
	TEntityUID TankA;
	TEntityUID TankB;

	// Other scene elements
	const int NumLights = 2;
	CLight* Lights[NumLights];
	SColourRGBA AmbientLight;
	CCamera* MainCamera;
	CCamera* CurrentCamera;
	int m_CurrentChaseTank;
	bool tankComplexInfo = false;

	// Sum of recent update times and number of times in the sum - used to calculate
	// average over a given time period
	float SumUpdateTimes = 0.0f;
	int NumUpdateTimes = 0;
	float AverageUpdateTime = -1.0f; // Invalid value at first


	float gameSpeed = 1.0f;

	//-----------------------------------------------------------------------------
	// Scene management
	//-----------------------------------------------------------------------------

	// Creates the scene geometry
	bool SceneSetup()
	{
		//////////////////////////////////////////////
		// Prepare render methods

		InitialiseMethods();
		LevelParser.ParseFile("Entities.xml");

		/////////////////////////////
		// Camera / light setup

		// Set camera position and clip planes
		MainCamera = new CCamera(CVector3(0.0f, 30.0f, -100.0f), CVector3(ToRadians(15.0f), 0, 0));
		MainCamera->SetNearFarClip(1.0f, 20000.0f);

		CurrentCamera = MainCamera;

		// Sunlight and light in building
		Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
		Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);

		// Ambient light level
		AmbientLight = SColourRGBA(0.6f, 0.6f, 0.6f, 1.0f);

		return true;
	}


	// Release everything in the scene
	void SceneShutdown()
	{
		// Release render methods
		ReleaseMethods();

		// Release lights
		for (int light = NumLights - 1; light >= 0; --light)
		{
			delete Lights[light];
		}

		// Release camera
		delete MainCamera;

		// Destroy all entities
		EntityManager.DestroyAllEntities();
		EntityManager.DestroyAllTemplates();
	}


	//-----------------------------------------------------------------------------
	// Game Helper functions
	//-----------------------------------------------------------------------------

	// Get UID of tank A (team 0) or B (team 1)
	TEntityUID GetTankUID(int team)
	{
		return (team == 0) ? TankA : TankB;
	}


	//-----------------------------------------------------------------------------
	// Game loop functions
	//-----------------------------------------------------------------------------

	// Draw one frame of the scene
	void RenderScene(float updateTime)
	{
		if (CurrentCamera == NULL)
		{
			CurrentCamera = MainCamera;
		}
		// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
		D3D10_VIEWPORT vp;
		vp.Width = ViewportWidth;
		vp.Height = ViewportHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		g_pd3dDevice->RSSetViewports(1, &vp);

		// Select the back buffer and depth buffer to use for rendering
		g_pd3dDevice->OMSetRenderTargets(1, &BackBufferRenderTarget, DepthStencilView);

		// Clear previous frame from back buffer and depth buffer
		g_pd3dDevice->ClearRenderTargetView(BackBufferRenderTarget, &AmbientLight.r);
		g_pd3dDevice->ClearDepthStencilView(DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0);

		// Update camera aspect ratio based on viewport size - for better results when changing window size
		CurrentCamera->SetAspect(static_cast<TFloat32>(ViewportWidth) / ViewportHeight);

		// Set camera and light data in shaders
		CurrentCamera->CalculateMatrices();
		SetCamera(CurrentCamera);
		SetAmbientLight(AmbientLight);
		SetLights(&Lights[0]);

		// Render entities and draw on-screen text
		EntityManager.RenderAllEntities();
		RenderSceneText(updateTime);

		// Present the backbuffer contents to the display
		SwapChain->Present(0, 0);
	}


	// Render a single text string at the given position in the given colour, may optionally centre it
	void RenderText(const string& text, int X, int Y, float r, float g, float b, bool centre = false)
	{
		RECT rect;
		if (!centre)
		{
			SetRect(&rect, X, Y, 0, 0);
			OSDFont->DrawText(NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR(r, g, b, 1.0f));
		}
		else
		{
			SetRect(&rect, X - 100, Y, X + 100, 0);
			OSDFont->DrawText(NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR(r, g, b, 1.0f));
		}
	}

	// Render on-screen text each frame
	void RenderSceneText(float updateTime)
	{
		stringstream outText;
		CVector2 pixel;

		std::vector<TEntityUID> tankList = EntityManager.GetList("Tank");

		for (int i = 0; i < tankList.size(); i++)
		{
			CTankEntity* tank = dynamic_cast<CTankEntity*>(EntityManager.GetEntity(tankList[i]));

			if (tankComplexInfo)
			{
				CVector3 pos = EntityManager.GetEntity(tankList[i])->Position();
				pos.y += 6;

				//if (CurrentCamera->PixelFromWorldPt(&pos, ViewportWidth, ViewportHeight, &X, &Y))
				if (CurrentCamera->PixelFromWorldPt(&pixel, pos, ViewportWidth, ViewportHeight))
				{
					outText << "Tank HP: " << tank->GetHP() << " \nTank State: " << tank->GetState() << " \nShells Fired: " << tank->GetShellsFired() << " \nAmmo : " << tank->GetAmmo();
				}
			}
			else
			{
				CVector3 pos = EntityManager.GetEntity(tankList[i])->Position();
				//pos.x += 3;
				pos.y += 3;

				if (CurrentCamera->PixelFromWorldPt(&pixel, pos, ViewportWidth, ViewportHeight))
				{
					outText << "Tank: " << EntityManager.GetEntity(tankList[i])->GetName();
				}
			}

			float r, g, b;
			if (tank->GetTeam() == 0)
			{
				r = 1.0f;
				g = 0.0f;
				b = 0.0f;
			}
			else
			{
				r = 0.0f;
				g = 1.0f;
				b = 0.0f;
			}

			RenderText(outText.str(), pixel.x, pixel.y, r, g, b);
			outText.str("");
		}

		// Accumulate update times to calculate the average over a given period
		SumUpdateTimes += updateTime;
		++NumUpdateTimes;
		if (SumUpdateTimes >= UpdateTimePeriod)
		{
			AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
			SumUpdateTimes = 0.0f;
			NumUpdateTimes = 0;
		}

		// Write FPS text string
		if (AverageUpdateTime >= 0.0f)
		{
			outText << "Frame Time: " << AverageUpdateTime * 1000.0f << "ms" << endl << "FPS:" << 1.0f / AverageUpdateTime;
			RenderText(outText.str(), 2, 2, 0.0f, 0.0f, 0.0f);
			RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
			outText.str("");

			outText << "Ammo Crate Timer: " << AmmoDropTimer << endl;
			RenderText(outText.str(), 2, 30, 0.0f, 0.0f, 0.0f);
			RenderText(outText.str(), 0, 30, 1.0f, 1.0f, 0.0f);
			outText.str("");
		}
	}


	// Update the scene between rendering
	void UpdateScene(float updateTime)
	{

		updateTime *= gameSpeed;

		if (CurrentCamera == NULL)
		{
			CurrentCamera = MainCamera;
		}
		// Call all entity update functions
		EntityManager.UpdateAllEntities(updateTime);

		if (CurrentCamera == NULL)
		{
			CurrentCamera = MainCamera;
		}

		CEntity* ammocrate = EntityManager.GetEntity("AmmoCrate");

		if (AmmoDropClaimed == false && ammocrate == NULL)
		{
			AmmoDropClaimed = true;
			AmmoDropTimer = 20.0f;
		}
		if (AmmoDropTimer <= 0.0f && AmmoDropClaimed == true)
		{
			TEntityUID AmmoCrate = EntityManager.CreateAmmoCrate("AmmoCrate", "AmmoCrate", CVector3(0.0f, 30.0f, 0.0f), CVector3(0.0f, ToRadians(0.0f), 0.0f), CVector3(0.2f, 0.2f, 0.2f));
			AmmoDropClaimed = false;

			std::vector<TEntityUID> m_TanksList = EntityManager.GetList("Tank");
		}
		else
		{
			AmmoDropTimer -= updateTime;
			if (AmmoDropTimer < 0.0f)
			{
				AmmoDropTimer = 0.0f;
			}
		}

		// Set camera speeds
		// Key F1 used for full screen toggle
		if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
		if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;
		if (KeyHit(Key_1))
		{
			std::vector<TEntityUID> m_TanksList = EntityManager.GetList("Tank");

			SMessage start;
			start.type = Msg_Start;
			for (int i = 0; i < m_TanksList.size(); i++)
			{
				Messenger.SendMessageA(m_TanksList[i], start);
			}
		}
		if (KeyHit(Key_2))
		{
			std::vector<TEntityUID> m_TanksList = EntityManager.GetList("Tank");

			SMessage stop;
			stop.type = Msg_Stop;
			for (int i = 0; i < m_TanksList.size(); i++)
			{
				Messenger.SendMessageA(m_TanksList[i], stop);
			}
		}
		if (KeyHit(Key_0))
		{
			tankComplexInfo = !tankComplexInfo;
		}
		if (KeyHit(Key_3))
		{
			std::vector<TEntityUID> m_TanksList = EntityManager.GetList("Tank");

			m_CurrentChaseTank = (m_CurrentChaseTank + 1) % m_TanksList.size();

			CurrentCamera = dynamic_cast<CTankEntity*>(EntityManager.GetEntity(m_TanksList[m_CurrentChaseTank]))->GetCamera();
		}
		if (KeyHit(Key_4))
		{
			CurrentCamera = MainCamera;
		}

		if (KeyHit(Mouse_LButton))
		{
			std::vector<TEntityUID> m_TanksList = EntityManager.GetList("Tank");
			CEntity* NearestTank = NULL;
			CVector2 pixel; // the pixel on the screen from the  tanks world space
			float clickAccuracy = 50; // how close to the tank must be clicked 
			float clickDistance; // the distance between the tanks pixel pos on the screen, and the pixel clicked
			/*Loops through all tanks and finds the nearest the mouse*/


			for (int i = 0; i < m_TanksList.size(); i++)
			{
				if (MainCamera->PixelFromWorldPt(&pixel, EntityManager.GetEntity(m_TanksList[i])->Position(), ViewportWidth, ViewportHeight))
				{
					clickDistance = Distance(mousePixelVector, pixel);
					if (clickDistance < clickAccuracy)
					{
						NearestTank = EntityManager.GetEntity(m_TanksList[i]);			
						CurrentCamera = dynamic_cast<CTankEntity*>(EntityManager.GetEntity(m_TanksList[i]))->GetCamera();
					}

				}
			}
		}

		if (KeyHit(Key_Z) && gameSpeed > 0.33)
		{
			gameSpeed -= 0.1f;
		}
		else if (KeyHit(Key_X) && gameSpeed < 3.0f)
		{
			gameSpeed += 0.1f;
		}

		// Move the camera
		CurrentCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
			CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime);
	}

	bool SphereToBox(float pointX, float pointY, float pointZ, float cubeXLength, float cubeYLength, float cubeZLength, float cubeXPos, float cubeYPos, float cubeZPos, float sphereRadius)
	{
		float minX = cubeXPos - (cubeXLength / 2.0f);
		float maxX = cubeXPos + (cubeXLength / 2.0f);
		float minY = cubeYPos - (cubeYLength / 2.0f);
		float maxY = cubeYPos + (cubeYLength / 2.0f);
		float minZ = cubeZPos - (cubeZLength / 2.0f);
		float maxZ = cubeZPos + (cubeZLength / 2.0f);



		if ((pointX > minX - sphereRadius && pointX < maxX + sphereRadius) &&
			(pointZ > minZ - sphereRadius && pointZ < maxZ + sphereRadius) &&
			(pointY > minY - sphereRadius && pointY < maxY + sphereRadius))
		{
			return true;
		}
		else
		{
			return false;
		}

	}
} // namespace gen
