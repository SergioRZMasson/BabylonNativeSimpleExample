#include <Windows.h>
#include <Windowsx.h>
#include <Shlwapi.h>
#include <filesystem>
#include <stdio.h>

#include <Babylon/AppRuntime.h>
#include <Babylon/Graphics/Device.h>
#include <Babylon/ScriptLoader.h>
#include <Babylon/Plugins/NativeEngine.h>
#include <Babylon/Plugins/NativeOptimizations.h>
#include <Babylon/Plugins/NativeInput.h>
#include <Babylon/Polyfills/Console.h>
#include <Babylon/Polyfills/Window.h>
#include <Babylon/Polyfills/XMLHttpRequest.h>
#include <Babylon/Polyfills/Canvas.h>

std::unique_ptr<Babylon::AppRuntime> runtime {};
std::unique_ptr<Babylon::Graphics::Device> device {};
std::unique_ptr<Babylon::Graphics::DeviceUpdate> update {};
Babylon::Plugins::NativeInput* nativeInput {};
std::unique_ptr<Babylon::Polyfills::Canvas> nativeCanvas {};
bool minimized = false;

LRESULT CALLBACK WndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

int main()
{
	 //-------------- Win32 application startup ---------------
	 const char CLASS_NAME[] = "Babylon Native Class";

	 auto hInstance = GetModuleHandle( NULL );

	 WNDCLASS wc = { };
	 wc.lpfnWndProc = WndProc;
	 wc.hInstance = hInstance;
	 wc.lpszClassName = CLASS_NAME;

	 RegisterClass( &wc );

	 HWND hwnd = CreateWindowEx( 0, CLASS_NAME, "Babylon Native Sample", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL );

	 if( hwnd == NULL )
		  return 0;

	 ShowWindow( hwnd, 1 );
	 UpdateWindow( hwnd );
	 EnableMouseInPointer( true );

	 //-------------- Initialize Babylon ----------------
	 RECT rect;
	 if( !GetClientRect( hwnd, &rect ) )
		  return 0;

	 auto width = rect.right - rect.left;
	 auto height = rect.bottom - rect.top;

	 //Create Babylon graphics infrastructure.
	 Babylon::Graphics::WindowConfiguration graphicsConfig {};
	 graphicsConfig.Window = hwnd;
	 graphicsConfig.Width = width;
	 graphicsConfig.Height = height;
	 graphicsConfig.MSAASamples = 4;

	 device = Babylon::Graphics::Device::Create( graphicsConfig );

	 //Update is used to call to the graphics thread.
	 update = std::make_unique<Babylon::Graphics::DeviceUpdate>( device->GetUpdate( "update" ) );

	 device->StartRenderingCurrentFrame();
	 update->Start();

	 //Create JS Runtime.
	 runtime = std::make_unique<Babylon::AppRuntime>();

	 //Add native implementations to the JS runtime.
	 runtime->Dispatch( []( Napi::Env env ) {
		  device->AddToJavaScript( env );

		  Babylon::Polyfills::Console::Initialize( env, []( const char* message, auto ) {
				OutputDebugStringA( message );
		  } );

		  Babylon::Polyfills::Window::Initialize( env );
		  Babylon::Polyfills::XMLHttpRequest::Initialize( env );
		  nativeCanvas = std::make_unique<Babylon::Polyfills::Canvas>( Babylon::Polyfills::Canvas::Initialize( env ) );

		  Babylon::Plugins::NativeEngine::Initialize( env );
		  Babylon::Plugins::NativeOptimizations::Initialize( env );

		  nativeInput = &Babylon::Plugins::NativeInput::CreateForJavaScript( env );
	 } );

	 //Load scripts
	 Babylon::ScriptLoader loader { *runtime };
	 loader.Eval( "document = {}", "" );
	 loader.LoadScript( "app:///Scripts/ammo.js" );
	 // Commenting out recast.js for now because v8jsi is incompatible with asm.js.
	 // loader.LoadScript("app:///Scripts/recast.js");
	 loader.LoadScript( "app:///Scripts/babylon.max.js" );
	 loader.LoadScript( "app:///Scripts/babylonjs.loaders.js" );
	 loader.LoadScript( "app:///Scripts/babylonjs.materials.js" );
	 loader.LoadScript( "app:///Scripts/babylon.gui.js" );
	 loader.LoadScript( "app:///Scripts/meshwriter.min.js" );
	 loader.LoadScript( "app:///Scripts/app.js" );

	 //-------------- Game loop -----------------

	 MSG msg {};

	 // Main message loop:
	 while( msg.message != WM_QUIT )
	 {
		  BOOL result;

		  if( minimized )
		  {
				result = GetMessage( &msg, nullptr, 0, 0 );
		  }
		  else
		  {
				if( device )
				{
					 update->Finish();
					 device->FinishRenderingCurrentFrame();
					 device->StartRenderingCurrentFrame();
					 update->Start();
				}

				result = PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) && msg.message != WM_QUIT;
		  }

		  if( result )
		  {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
		  }
	 }

	 return ( int ) msg.wParam;
}

void ProcessMouseButtons( tagPOINTER_BUTTON_CHANGE_TYPE changeType, int x, int y )
{
	 switch( changeType )
	 {
		  case POINTER_CHANGE_FIRSTBUTTON_DOWN:
				nativeInput->MouseDown( Babylon::Plugins::NativeInput::LEFT_MOUSE_BUTTON_ID, x, y );
				break;
		  case POINTER_CHANGE_FIRSTBUTTON_UP:
				nativeInput->MouseUp( Babylon::Plugins::NativeInput::LEFT_MOUSE_BUTTON_ID, x, y );
				break;
		  case POINTER_CHANGE_SECONDBUTTON_DOWN:
				nativeInput->MouseDown( Babylon::Plugins::NativeInput::RIGHT_MOUSE_BUTTON_ID, x, y );
				break;
		  case POINTER_CHANGE_SECONDBUTTON_UP:
				nativeInput->MouseUp( Babylon::Plugins::NativeInput::RIGHT_MOUSE_BUTTON_ID, x, y );
				break;
		  case POINTER_CHANGE_THIRDBUTTON_DOWN:
				nativeInput->MouseDown( Babylon::Plugins::NativeInput::MIDDLE_MOUSE_BUTTON_ID, x, y );
				break;
		  case POINTER_CHANGE_THIRDBUTTON_UP:
				nativeInput->MouseUp( Babylon::Plugins::NativeInput::MIDDLE_MOUSE_BUTTON_ID, x, y );
				break;
	 }
}

void UpdateWindowSize( size_t width, size_t height )
{
	 device->UpdateSize( width, height );
}

void Uninitialize()
{
	 if( device )
	 {
		  update->Finish();
		  device->FinishRenderingCurrentFrame();
	 }

	 nativeInput = {};
	 runtime.reset();
	 nativeCanvas.reset();
	 update.reset();
	 device.reset();
}

void RefreshBabylon( HWND hWnd )
{
	 Uninitialize();

	 RECT rect;
	 if( !GetClientRect( hWnd, &rect ) )
	 {
		  return;
	 }

	 auto width = static_cast< size_t >( rect.right - rect.left );
	 auto height = static_cast< size_t >( rect.bottom - rect.top );

	 Babylon::Graphics::WindowConfiguration graphicsConfig {};
	 graphicsConfig.Window = hWnd;
	 graphicsConfig.Width = width;
	 graphicsConfig.Height = height;
	 graphicsConfig.MSAASamples = 4;

	 device = Babylon::Graphics::Device::Create( graphicsConfig );
	 update = std::make_unique<Babylon::Graphics::DeviceUpdate>( device->GetUpdate( "update" ) );
	 device->StartRenderingCurrentFrame();
	 update->Start();

	 runtime = std::make_unique<Babylon::AppRuntime>();

	 runtime->Dispatch( []( Napi::Env env ) {
		  device->AddToJavaScript( env );

		  Babylon::Polyfills::Console::Initialize( env, []( const char* message, auto ) {
				OutputDebugStringA( message );
		  } );

		  Babylon::Polyfills::Window::Initialize( env );

		  Babylon::Polyfills::XMLHttpRequest::Initialize( env );
		  nativeCanvas = std::make_unique <Babylon::Polyfills::Canvas>( Babylon::Polyfills::Canvas::Initialize( env ) );

		  Babylon::Plugins::NativeEngine::Initialize( env );

		  Babylon::Plugins::NativeOptimizations::Initialize( env );

		  nativeInput = &Babylon::Plugins::NativeInput::CreateForJavaScript( env );
	 } );

	 Babylon::ScriptLoader loader { *runtime };
	 loader.Eval( "document = {}", "" );
	 loader.LoadScript( "app:///Scripts/ammo.js" );
	 // Commenting out recast.js for now because v8jsi is incompatible with asm.js.
	 // loader.LoadScript("app:///Scripts/recast.js");
	 loader.LoadScript( "app:///Scripts/babylon.max.js" );
	 loader.LoadScript( "app:///Scripts/babylonjs.loaders.js" );
	 loader.LoadScript( "app:///Scripts/babylonjs.materials.js" );
	 loader.LoadScript( "app:///Scripts/babylon.gui.js" );
	 loader.LoadScript( "app:///Scripts/meshwriter.min.js" );
	 loader.LoadScript( "app:///Scripts/app.js" );
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	 switch( message )
	 {
		  case WM_SIZE:
		  {
				if( device )
				{
					 auto width = static_cast< size_t >( LOWORD( lParam ) );
					 auto height = static_cast< size_t >( HIWORD( lParam ) );
					 UpdateWindowSize( width, height );
				}
				break;
		  }
		  case WM_DESTROY:
		  {
				Uninitialize();
				PostQuitMessage( 0 );
				break;
		  }
		  case WM_POINTERWHEEL:
		  {
				if( nativeInput != nullptr )
				{
					 nativeInput->MouseWheel( Babylon::Plugins::NativeInput::MOUSEWHEEL_Y_ID, -GET_WHEEL_DELTA_WPARAM( wParam ) );
				}
				break;
		  }
		  case WM_POINTERDOWN:
		  {
				if( nativeInput != nullptr )
				{
					 POINTER_INFO info;
					 auto pointerId = GET_POINTERID_WPARAM( wParam );

					 if( GetPointerInfo( pointerId, &info ) )
					 {
						  auto x = GET_X_LPARAM( lParam );
						  auto y = GET_Y_LPARAM( lParam );

						  if( info.pointerType == PT_MOUSE )
						  {
								ProcessMouseButtons( info.ButtonChangeType, x, y );
						  }
						  else
						  {
								nativeInput->TouchDown( pointerId, x, y );
						  }
					 }
				}
				break;
		  }
		  case WM_POINTERUPDATE:
		  {
				if( nativeInput != nullptr )
				{
					 POINTER_INFO info;
					 auto pointerId = GET_POINTERID_WPARAM( wParam );

					 if( GetPointerInfo( pointerId, &info ) )
					 {
						  auto x = GET_X_LPARAM( lParam );
						  auto y = GET_Y_LPARAM( lParam );

						  if( info.pointerType == PT_MOUSE )
						  {
								ProcessMouseButtons( info.ButtonChangeType, x, y );
								nativeInput->MouseMove( x, y );
						  }
						  else
						  {
								nativeInput->TouchMove( pointerId, x, y );
						  }
					 }
				}
				break;
		  }
		  case WM_POINTERUP:
		  {
				if( nativeInput != nullptr )
				{
					 POINTER_INFO info;
					 auto pointerId = GET_POINTERID_WPARAM( wParam );

					 if( GetPointerInfo( pointerId, &info ) )
					 {
						  auto x = GET_X_LPARAM( lParam );
						  auto y = GET_Y_LPARAM( lParam );

						  if( info.pointerType == PT_MOUSE )
						  {
								ProcessMouseButtons( info.ButtonChangeType, x, y );
						  }
						  else
						  {
								nativeInput->TouchUp( pointerId, x, y );
						  }
					 }
				}
				break;
		  }
		  case WM_KEYDOWN:
		  {
				if( wParam == 'R' )
				{
					 RefreshBabylon( hWnd );
				}
				break;
		  }
		  default:
		  {
				return DefWindowProc( hWnd, message, wParam, lParam );
		  }
	 }
	 return 0;
}