#include "handmade.h"

void GameOutputSound(game_state* GameState, game_sound_output_buffer* SoundBuffer, int ToneHz)
{
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16* SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
        // TODO(casey): Draw this out for people
        real32 SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
        if (GameState->tSine > 2.0f * Pi32)
        {
            GameState->tSine -= 2.0f * Pi32;
        }
    }
}

void RenderWeirdGradient(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
    // TODO(casey): Let's see what the optimizer does

    uint8* Row = (uint8*)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; Y++)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0; X < Buffer->Width; X++)
        {
            uint8 Blue = (uint8)(X + BlueOffset);
            uint8 Green = (uint8)(Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

void RenderMouse(game_offscreen_buffer* Buffer, int32 MouseX, int32 MouseY)
{
    uint8* EndOfBuffer = 
        (uint8*)Buffer->Memory + 
        Buffer->BytesPerPixel * Buffer->Width * Buffer->Height;

    uint32 Color = 0xFFFFFFFF;
    int32 Top = MouseY;
    int32 Bottom = MouseY + 10;

    for (int32 X = MouseX; X < MouseX + 10; X++)
    {
        uint8* Pixel = ((uint8*)Buffer->Memory + X * Buffer->BytesPerPixel + Top * Buffer->Pitch);
        for (int Y = Top; Y < Bottom; Y++)
        {
            if ((Pixel >= Buffer->Memory) && (Pixel < EndOfBuffer))
            {
                *(uint32*)Pixel = Color;
                Pixel += Buffer->Pitch;
            }
        }
    }
}

extern "C" __declspec(dllexport) GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state* GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        const char* Filename = __FILE__;

        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Context, Filename);
        if (File.Contents)
        {
            Memory->DEBUGPlatformWriteEntireFile(Context, "test.out", File.ContentsSize, File.Contents);
            Memory->DEBUGPlatformFreeFileMemory(Context, File.Contents);
        }

        GameState->ToneHz = 512;
        GameState->tSine = 0.0f;

        // TODO(casey): This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
    }

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input* Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            // NOTE(casey): Use analog movement tuning
            GameState->BlueOffset += (int)(4.0f * Controller->StickAverageX);
            GameState->ToneHz = 512 + (int)(128.0f * Controller->StickAverageY);
        }
        else
        {
            // NOTE(casey): Use digital movement tuning
            if (Controller->MoveLeft.EndedDown)
            {
                GameState->BlueOffset -= 1;
            }

            if (Controller->MoveRight.EndedDown)
            {
                GameState->BlueOffset += 1;
            }
        }

        // Input.AButtonEndedDown;
        // Input.AButtonHalfTransitionCount;
        if (Controller->ActionDown.EndedDown)
        {
            GameState->GreenOffset += 1;
        }
    }

    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);

    RenderMouse(Buffer, Input->MouseX, Input->MouseY);

    for (int ButtonIndex = 0; ButtonIndex < ArrayCount(Input->MouseButtons); ++ButtonIndex)
    {
        if (Input->MouseButtons[ButtonIndex].EndedDown)
        {
            RenderMouse(Buffer, 10 + 20 * ButtonIndex, 10);
        }
    }
}

extern "C" __declspec(dllexport) GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}
