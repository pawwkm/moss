#include "buffers.h"

#include <ctype.h>
#include <string.h>

static void c_multiline(Line* line, Token* token, uint16_t* index, const uint16_t start_index, bool* const continue_multiline_comment)
{
    while (*index != line->characters_length)
    {
        if (*index + 1 <= line->characters_length && line->characters[*index] == '*' && line->characters[*index + 1] == '/')
            break;

        (*index)++;
    }

    token->tag = Token_Tag_comment;
    if (*index + 1 <= line->characters_length && line->characters[*index] == '*' && line->characters[*index + 1] == '/')
    {
        *index += 2;
        token->characters_length = *index - start_index;
        *continue_multiline_comment = false;
    }
    else
    {
        token->characters_length = line->characters_length - start_index;
        *continue_multiline_comment = true;

        return;
    }
}

static bool is_c_hex_quad(const Line* const line, uint16_t index)
{
    return index + 3 <= line->characters_length  &&
           isxdigit(line->characters[index])     &&
           isxdigit(line->characters[index + 1]) &&
           isxdigit(line->characters[index + 2]) &&
           isxdigit(line->characters[index + 3]);
}

// true if the next code point(s) is a valid or forms a valid escape sequence.
static bool lex_c_character(Line* line, uint16_t* index, bool is_string)
{
    if (*index + 1 <= line->characters_length && line->characters[*index] == '\\')
    {
        switch (line->characters[++(*index)])
        {
            case '"':
                if (is_string) 
                {
                    (*index)++;
                    return true;
                }
                else
                    return false;

            case '\'':
                if (!is_string)
                {
                    (*index)++;
                    return true;
                }
                else
                    return false;

            case '?':
            case '\\':
            case 'a':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case 'v':
                (*index)++;
                return true;

            // Octal.
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                if (*index != line->characters_length && line->characters[*index] >= '0' && line->characters[*index] <= '7')
                    (*index)++;

                if (*index != line->characters_length && line->characters[*index] >= '0' && line->characters[*index] <= '7')
                    (*index)++;

                return true;

            case 'x':
                (*index)++;
                if (*index != line->characters_length && isxdigit(line->characters[*index]))
                {
                    (*index)++;
                    while (*index != line->characters_length && isxdigit(line->characters[*index]))
                        (*index)++;

                    return true;
                }
                else
                    return false;

            case 'u':
                if (is_c_hex_quad(line, *index))
                {
                    *index += 3;
                    return true;
                }
                else
                    return false;

            case 'U':
                if (is_c_hex_quad(line, *index) && is_c_hex_quad(line, *index + 3))
                {
                    *index += 7;
                    return true;
                }
                else
                    return false;

            default:
                return false;
        }
    }
    else if (*index != line->characters_length)
    {
        (*index)++;
        return true;
    }
    else 
        return false;
}

static bool last_token_is_preceeded_by_pound_sign(const Line* const line)
{
    for (int32_t i = line->tokens_length - 2; i >= 0; i--)
    {
        const Token* const token = &line->tokens[i];
        if (token->tag == Token_Tag_white_space)
            continue;
        else
            return token->characters_length == 1 && token->characters[0] == '#';
    }

    return false;
}

void lexical_analyze_c(Line* line, bool* continue_multiline_comment)
{
    // These are sorted by length ascending order which 
    // is used to bail out of a loop early if next keyword
    // is longer than expected.
    static String_Slice keywords[47] =
    {
        { 2,  "if"             },
        { 2,  "do"             },
        { 3,  "int"            },
        { 3,  "for"            },
        { 4,  "case"           },
        { 4,  "char"           },
        { 4,  "auto"           },
        { 4,  "goto"           },
        { 4,  "long"           },
        { 4,  "else"           },
        { 4,  "enum"           },
        { 4,  "void"           },
        { 5,  "break"          },
        { 5,  "const"          },
        { 5,  "float"          },
        { 5,  "union"          },
        { 5,  "short"          },
        { 5,  "while"          },
        { 5,  "_Bool"          },
        { 6,  "double"         },
        { 6,  "extern"         },
        { 6,  "inline"         },
        { 6,  "return"         },
        { 6,  "signed"         },
        { 6,  "sizeof"         },
        { 6,  "static"         },
        { 6,  "struct"         },
        { 6,  "switch"         },
        { 7,  "default"        },
        { 7,  "_Atomic"        },
        { 7,  "typedef"        },
        { 8,  "continue"       },
        { 8,  "register"       },
        { 8,  "restrict"       },
        { 8,  "unsigned"       },
        { 8,  "volatile"       },
        { 8,  "_Alignas"       },
        { 8,  "_Alignof"       },
        { 8,  "_Complex"       },
        { 8,  "_Generic"       },
        { 9,  "_Noreturn"      },
        { 11, "_Decimal128"    },
        { 10, "_Decimal32"     },
        { 10, "_Decimal64"     },
        { 10, "_Imaginary"     },
        { 13, "_Thread_local"  },
        { 14, "_Static_assert" }
    };

    // These are sorted by length ascending order which 
    // is used to bail out of a loop early if next type
    // is longer than expected.
    static String_Slice predefined_c_types[] =
    {
        { 2,  "tm" },
        { 4,  "bool" },
        { 4,  "FILE" },
        { 5,  "Sint8" },
        { 5,  "Uint8" },
        { 6,  "Sint16" },
        { 6,  "Sint32" },
        { 6,  "Sint64" },
        { 6,  "Uint16" },
        { 6,  "Uint32" },
        { 6,  "Uint64" },
        { 7,  "SDL_sem" },
        { 7,  "va_list" },
        { 8,  "MD5UINT4" },
        { 8,  "NSWindow" },
        { 8,  "SDL_bool" },
        { 8,  "SDL_cond" },
        { 8,  "SDL_Rect" },
        { 8,  "timespec" },
        { 8,  "UIWindow" },
        { 9,  "SDL_Color" },
        { 9,  "SDL_Event" },
        { 9,  "SDL_FRect" },
        { 9,  "SDL_mutex" },
        { 9,  "SDL_Point" },
        { 9,  "SDL_RWops" },
        { 9,  "SDL_TLSID" },
        { 10, "SDL_Finger" },
        { 10, "SDL_FPoint" },
        { 10, "SDL_GLattr" },
        { 10, "SDL_Haptic" },
        { 10, "SDL_Keymod" },
        { 10, "SDL_Keysym" },
        { 10, "SDL_Locale" },
        { 10, "SDL_Sensor" },
        { 10, "SDL_Thread" },
        { 10, "SDL_Window" },
        { 11, "SDL_KeyCode" },
        { 11, "SDL_OSEvent" },
        { 11, "SDL_Palette" },
        { 11, "SDL_Surface" },
        { 11, "SDL_Texture" },
        { 11, "SDL_TimerID" },
        { 11, "SDL_TouchID" },
        { 11, "SDL_version" },
        { 12, "ID3D11Device" },
        { 12, "SDL_AudioCVT" },
        { 12, "SDL_Joystick" },
        { 12, "SDL_Renderer" },
        { 12, "SDL_Scancode" },
        { 12, "SDL_SensorID" },
        { 12, "SDL_SpinLock" },
        { 12, "SDL_SysWMmsg" },
        { 12, "SDL_SysWMmsg" },
        { 12, "SDL_threadID" },
        { 13, "SDL_AudioSpec" },
        { 13, "SDL_BlendMode" },
        { 13, "SDL_DropEvent" },
        { 13, "SDL_errorcode" },
        { 13, "SDL_EventType" },
        { 13, "SDL_GestureID" },
        { 13, "SDL_GLContext" },
        { 13, "SDL_GLprofile" },
        { 13, "SDL_main_func" },
        { 13, "SDL_MetalView" },
        { 13, "SDL_PixelType" },
        { 13, "SDL_QuitEvent" },
        { 13, "SDL_ScaleMode" },
        { 13, "SDL_SysWMinfo" },
        { 13, "SDL_UserEvent" },
        { 14, "SDL_ArrayOrder" },
        { 14, "SDL_AssertData" },
        { 14, "SDL_HapticRamp" },
        { 14, "SDL_PowerState" },
        { 14, "SDL_SensorType" },
        { 14, "SDL_SYSWM_TYPE" },
        { 14, "SDL_SysWMEvent" },
        { 14, "SDL_WinRT_Path" },
        { 15, "SDL_AssertState" },
        { 15, "SDL_AudioFilter" },
        { 15, "SDL_AudioFormat" },
        { 15, "SDL_AudioStream" },
        { 15, "SDL_BitmapOrder" },
        { 15, "SDL_BlendFactor" },
        { 15, "SDL_CommonEvent" },
        { 15, "SDL_DisplayMode" },
        { 15, "SDL_eventaction" },
        { 15, "SDL_JoyHatEvent" },
        { 15, "SDL_LogCategory" },
        { 15, "SDL_LogPriority" },
        { 15, "SDL_PackedOrder" },
        { 15, "SDL_PixelFormat" },
        { 15, "SDL_SensorEvent" },
        { 15, "SDL_WindowEvent" },
        { 15, "SDL_WindowFlags" },
        { 15, "WindowShapeMode" },
        { 16, "IDirect3DDevice9" },
        { 16, "SDL_DisplayEvent" },
        { 16, "SDL_HapticCustom" },
        { 16, "SDL_HapticEffect" },
        { 16, "SDL_HintCallback" },
        { 16, "SDL_HintPriority" },
        { 16, "SDL_JoyAxisEvent" },
        { 16, "SDL_JoyBallEvent" },
        { 16, "SDL_JoystickGUID" },
        { 16, "SDL_JoystickType" },
        { 16, "SDL_PackedLayout" },
        { 16, "SDL_RendererFlip" },
        { 16, "SDL_RendererInfo" },
        { 16, "SDL_SystemCursor" },
        { 16, "UIViewController" },
        { 17, "SDL_AudioCallback" },
        { 17, "SDL_AudioDeviceID" },
        { 17, "SDL_GLcontextFlag" },
        { 17, "SDL_KeyboardEvent" },
        { 17, "SDL_RendererFlags" },
        { 17, "SDL_TextureAccess" },
        { 17, "SDL_TimerCallback" },
        { 17, "SDL_vulkanSurface" },
        { 17, "SDL_WindowEventID" },
        { 18, "SDL_BlendOperation" },
        { 18, "SDL_DisplayEventID" },
        { 18, "SDL_FlashOperation" },
        { 18, "SDL_GameController" },
        { 18, "SDL_HapticConstant" },
        { 18, "SDL_HapticPeriodic" },
        { 18, "SDL_JoyButtonEvent" },
        { 18, "SDL_JoyDeviceEvent" },
        { 18, "SDL_MessageBoxData" },
        { 18, "SDL_TextInputEvent" },
        { 18, "SDL_ThreadFunction" },
        { 18, "SDL_ThreadPriority" },
        { 18, "SDL_vulkanInstance" },
        { 18, "SDLTest_Md5Context" },
        { 19, "SDL_HapticCondition" },
        { 19, "SDL_HapticDirection" },
        { 19, "SDL_HapticLeftRight" },
        { 19, "SDL_MessageBoxColor" },
        { 19, "SDL_MessageBoxFlags" },
        { 19, "SDL_MouseWheelEvent" },
        { 19, "SDL_PixelFormatEnum" },
        { 19, "SDL_TextureModulate" },
        { 19, "SDL_TouchDeviceType" },
        { 19, "SDL_WindowShapeMode" },
        { 19, "SDLTest_CommonState" },
        { 20, "SDL_AssertionHandler" },
        { 20, "SDL_AudioDeviceEvent" },
        { 20, "SDL_MouseButtonEvent" },
        { 20, "SDL_MouseMotionEvent" },
        { 20, "SDL_TextEditingEvent" },
        { 20, "SDL_TouchFingerEvent" },
        { 21, "SDL_LogOutputFunction" },
        { 21, "SDL_MultiGestureEvent" },
        { 21, "SDL_WindowShapeParams" },
        { 21, "SDLTest_RandomContext" },
        { 22, "SDL_DisplayOrientation" },
        { 22, "SDL_DollarGestureEvent" },
        { 22, "SDL_GameControllerAxis" },
        { 22, "SDL_GameControllerType" },
        { 22, "SDL_JoystickPowerLevel" },
        { 22, "SDL_WindowsMessageHook" },
        { 22, "SDL_WinRT_DeviceFamily" },
        { 23, "pfnSDL_CurrentEndThread" },
        { 23, "SDL_ControllerAxisEvent" },
        { 23, "SDL_MessageBoxColorType" },
        { 23, "SDL_MouseWheelDirection" },
        { 23, "SDL_YUV_CONVERSION_MODE" },
        { 24, "SDL_GameControllerButton" },
        { 24, "SDL_GLcontextReleaseFlag" },
        { 24, "SDL_MessageBoxButtonData" },
        { 25, "pfnSDL_CurrentBeginThread" },
        { 25, "SDL_ControllerButtonEvent" },
        { 25, "SDL_ControllerDeviceEvent" },
        { 25, "SDL_ControllerSensorEvent" },
        { 25, "SDL_MessageBoxButtonFlags" },
        { 25, "SDL_MessageBoxColorScheme" },
        { 26, "SDL_GameControllerBindType" },
        { 27, "SDL_ControllerTouchpadEvent" },
        { 28, "SDL_GameControllerButtonBind" },
        { 30, "SDL_GLContextResetNotification" }
    };

    uint16_t index = 0;
    if (index == line->characters_length && *continue_multiline_comment)
        // This is an empty line inside of a multiline comment.
        // Return now to keep *continue_multiline_comment == true.
        return;

    while (index != line->characters_length)
    {
        const uint16_t start_index = index;

        Token* const token = add_token(line);
        token->characters = &line->characters[index];
        
        if (*continue_multiline_comment)
        {
            c_multiline(line, token, &index, start_index, continue_multiline_comment);
            if (*continue_multiline_comment)
                return;
        }
        else if (isspace(line->characters[index]))
        {
            while (index != line->characters_length && isspace(line->characters[index]))
                index++;

            token->characters_length = index - start_index;
            token->tag = Token_Tag_white_space;
        }
        else if (index + 1 != line->characters_length && line->characters[index] == '/' && line->characters[index + 1] == '/')
        {
            token->characters_length = line->characters_length - start_index;
            token->tag = Token_Tag_comment;

            break;
        }
        else if (index + 1 <= line->characters_length && line->characters[index] == '/' && line->characters[index + 1] == '*')
        {
            index++;
            
            c_multiline(line, token, &index, start_index, continue_multiline_comment);
            if (*continue_multiline_comment)
                return;
        }
        else if (line->characters[index] == '"' || index + 1 <= line->characters_length && line->characters[index] == 'L' && line->characters[index + 1] == '"')
        {
            if (line->characters[index] == 'L')
                index += 2;
            else
                index++;

            while (index != line->characters_length && line->characters[index] != '"')
            {
                if (!lex_c_character(line, &index, true))
                    break;
            }

            if (index != line->characters_length && line->characters[index] == '"')
            {
                index++;
                token->characters_length = index - start_index;
                token->tag = Token_Tag_string;
            }
            else
                token->characters_length = index - start_index;
        }
        else if (line->characters[index] == '\'' || index + 1 <= line->characters_length && line->characters[index] == 'L' && line->characters[index + 1] == '\'')
        {
            if (line->characters[index] == 'L')
                index += 2;
            else
                index++;

            if (index != line->characters_length && line->characters[index] == '\'')
            {
                index++;
                token->characters_length = index - start_index;
            }
            else
            {
                while (index != line->characters_length && line->characters[index] != '\'')
                {
                    if (!lex_c_character(line, &index, false))
                        break;
                }

                if (index != line->characters_length && line->characters[index] == '\'')
                {
                    index++;
                    token->characters_length = index - start_index;
                    token->tag = Token_Tag_string;
                }
                else
                    token->characters_length = index - start_index;
            }
        }
        else if (ispunct(line->characters[index]))
        {
            index++;
            token->characters_length = index - start_index;
        }
        else if (isalpha(line->characters[index]))
        {
            do
                index++;
            while (index != line->characters_length && (isalnum(line->characters[index]) || line->characters[index] == '_'));

            token->characters_length = index - start_index;
            for (uint8_t k = 0; k < sizeof(keywords) / sizeof(keywords[0]); k++)
            {
                String_Slice* const keyword = &keywords[k];
                if (keyword->characters_length > token->characters_length)
                    break;

                if (keyword->characters_length != token->characters_length)
                    continue;

                if (!strncmp(token->characters, keyword->characters, keyword->characters_length))
                {
                    if (k != 0 || !last_token_is_preceeded_by_pound_sign(line))
                        token->tag = Token_Tag_keyword;

                    break;
                }
            }

            if (token->tag == Token_Tag_keyword)
                continue;

            // If the identifier ends with _t it is probably a type.
            if (token->characters_length >= 2 && line->characters[index - 2] == '_' && line->characters[index - 1] == 't')
            {
                token->tag = Token_Tag_type;
                continue;
            }

            for (uint16_t t = 0; t < sizeof(predefined_c_types) / sizeof(predefined_c_types[0]); t++)
            {
                String_Slice* type = &predefined_c_types[t];
                if (type->characters_length > token->characters_length)
                    break;

                if (type->characters_length != token->characters_length)
                    continue;

                if (!memcmp(token->characters, type->characters, sizeof(type->characters[0]) * type->characters_length))
                {
                    token->tag = Token_Tag_type;
                    break;
                }
            }

            if (token->tag == Token_Tag_type)
                continue;

            // Owen style types.
            if (isupper(token->characters[0]))
            {
                uint16_t i = 0;
                do
                {
                    i++;
                    while (i != token->characters_length && (islower(token->characters[i]) || isdigit(token->characters[i])))
                        i++;

                    if (i + 1 <= token->characters_length && token->characters[i] == '_' && isupper(token->characters[i + 1]))
                        i++;
                    else
                        break;

                } while (isupper(token->characters[i]));
                
                if (i == token->characters_length)
                    token->tag = Token_Tag_type;
            }
        }
        else
        {
            while (index != line->characters_length && !isspace(line->characters[index]) && !ispunct(line->characters[index]))
                index++;

            token->characters_length = index - start_index;
        }
    }

    *continue_multiline_comment = false;
}
