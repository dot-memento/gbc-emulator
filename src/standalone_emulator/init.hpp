#pragma once

struct GLFWwindow;

void setupLogging();

GLFWwindow* initializeGlfw();
void initializeImGui();

void prepareImGuiFrame();
void renderImGuiFrame();

void terminateImGui();
void terminateGlfw();
