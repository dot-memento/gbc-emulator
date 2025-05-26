#pragma once

class SubWindow
{
public:
    SubWindow() = default;
    virtual ~SubWindow() {}

    SubWindow(const SubWindow&) = delete;
    SubWindow& operator=(const SubWindow&) = delete;

    virtual const char* getName() const = 0;
    virtual void draw() = 0;

    constexpr bool isOpened() const { return is_opened_; }
    constexpr void open() { is_opened_ = true; }
    constexpr void close() { is_opened_ = false; }

protected:
    bool is_opened_ = false;
};
