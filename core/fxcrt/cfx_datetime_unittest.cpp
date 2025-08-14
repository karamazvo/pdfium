// Copyright 2021 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_datetime.h"

#include "core/fxcrt/fake_time_test.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST_F(FakeTimeTest, Now) {
  CFX_DateTime dt = CFX_DateTime::Now();
  EXPECT_EQ(2020, dt.GetYear());
  EXPECT_EQ(4, dt.GetMonth());
  EXPECT_EQ(23, dt.GetDay());
  EXPECT_EQ(15, dt.GetHour());
  EXPECT_EQ(5, dt.GetMinute());
  EXPECT_EQ(21, dt.GetSecond());
  EXPECT_EQ(0, dt.GetMillisecond());
}

TEST(DateTime, IsLeapYear) {
  EXPECT_TRUE(FX_IsLeapYear(2000));
  EXPECT_TRUE(FX_IsLeapYear(2004));
  EXPECT_FALSE(FX_IsLeapYear(2100));
  EXPECT_FALSE(FX_IsLeapYear(2001));
}

TEST(DateTime, DaysInMonth) {
  EXPECT_EQ(29, FX_DaysInMonth(2000, 2));
  EXPECT_EQ(28, FX_DaysInMonth(2001, 2));
  EXPECT_EQ(31, FX_DaysInMonth(2000, 1));
  EXPECT_EQ(30, FX_DaysInMonth(2000, 4));
}

TEST(DateTime, Getters) {
  CFX_DateTime dt(2021, 5, 24, 12, 30, 0, 0);
  EXPECT_EQ(2021, dt.GetYear());
  EXPECT_EQ(5, dt.GetMonth());
  EXPECT_EQ(24, dt.GetDay());
  EXPECT_EQ(12, dt.GetHour());
  EXPECT_EQ(30, dt.GetMinute());
  EXPECT_EQ(0, dt.GetSecond());
  EXPECT_EQ(0, dt.GetMillisecond());
}

TEST(DateTime, SetDate) {
  CFX_DateTime dt;
  dt.SetDate(2021, 5, 24);
  EXPECT_EQ(2021, dt.GetYear());
  EXPECT_EQ(5, dt.GetMonth());
  EXPECT_EQ(24, dt.GetDay());
}

TEST(DateTime, SetTime) {
  CFX_DateTime dt;
  dt.SetTime(12, 30, 0, 0);
  EXPECT_EQ(12, dt.GetHour());
  EXPECT_EQ(30, dt.GetMinute());
  EXPECT_EQ(0, dt.GetSecond());
  EXPECT_EQ(0, dt.GetMillisecond());
}

TEST(DateTime, GetDayOfWeek) {
  CFX_DateTime dt(2021, 5, 24, 12, 30, 0, 0);
  EXPECT_EQ(1, dt.GetDayOfWeek());  // Monday
}

TEST(DateTime, OperatorEq) {
  CFX_DateTime dt1(2021, 5, 24, 12, 30, 0, 0);
  CFX_DateTime dt2(2021, 5, 24, 12, 30, 0, 0);
  CFX_DateTime dt3(2021, 5, 25, 12, 30, 0, 0);
  EXPECT_EQ(dt1, dt2);
  EXPECT_NE(dt1, dt3);
}

TEST(DateTime, Reset) {
  CFX_DateTime dt(2021, 5, 24, 12, 30, 0, 0);
  dt.Reset();
  EXPECT_EQ(0, dt.GetYear());
  EXPECT_EQ(0, dt.GetMonth());
  EXPECT_EQ(0, dt.GetDay());
  EXPECT_EQ(0, dt.GetHour());
  EXPECT_EQ(0, dt.GetMinute());
  EXPECT_EQ(0, dt.GetSecond());
  EXPECT_EQ(0, dt.GetMillisecond());
}

TEST(DateTime, IsSet) {
  CFX_DateTime dt;
  EXPECT_FALSE(dt.IsSet());
  dt.SetDate(2021, 5, 24);
  EXPECT_TRUE(dt.IsSet());
}
