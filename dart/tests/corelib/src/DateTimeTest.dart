// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

// Dart test program for Date.

class DateTest {
  // Tests if the time moves eventually forward.
  static void testNow() {
    var t1 = new Date.now();
    bool timeMovedForward = false;
    for (int i = 0; i < 1000000; i++) {
      var t2 = new Date.now();
      if (t1.value < t2.value) {
        timeMovedForward = true;
        break;
      }
    }
    Expect.equals(true, timeMovedForward);
  }

  static void testValue() {
    var dt1 = new Date.now();
    var value = dt1.value;
    var dt2 = new Date.fromEpoch(value, new TimeZone.local());
    Expect.equals(value, dt2.value);
  }

  static void testFarAwayDates() {
    Date dt = new Date.fromEpoch(1000000000000001, const TimeZone.utc());
    Expect.equals(33658, dt.year);
    Expect.equals(9, dt.month);
    Expect.equals(27, dt.day);
    Expect.equals(1, dt.hours);
    Expect.equals(46, dt.minutes);
    Expect.equals(40, dt.seconds);
    Expect.equals(1, dt.milliseconds);
    dt = new Date.fromEpoch(-1000000000000001, const TimeZone.utc());
    Expect.equals(-29719, dt.year);
    Expect.equals(4, dt.month);
    Expect.equals(5, dt.day);
    Expect.equals(22, dt.hours);
    Expect.equals(13, dt.minutes);
    Expect.equals(19, dt.seconds);
    Expect.equals(999, dt.milliseconds);
    // Same with local zone.
    dt = new Date.fromEpoch(1000000000000001, new TimeZone.local());
    Expect.equals(33658, dt.year);
    Expect.equals(9, dt.month);
    Expect.equals(true, dt.day == 27 || dt.day == 26);
    // Not much we can test for local hours.
    Expect.equals(true, dt.hours >= 0 && dt.hours < 24);
    // Timezones can have offsets down to 15 minutes.
    Expect.equals(true, dt.minutes % 15 == 46 % 15);
    Expect.equals(40, dt.seconds);
    Expect.equals(1, dt.milliseconds);
    dt = new Date.fromEpoch(-1000000000000001, new TimeZone.local());
    Expect.equals(-29719, dt.year);
    Expect.equals(4, dt.month);
    Expect.equals(true, 5 == dt.day || 6 == dt.day);
    // Not much we can test for local hours.
    Expect.equals(true, dt.hours >= 0 && dt.hours < 24);
    // Timezones can have offsets down to 15 minutes.
    Expect.equals(true, dt.minutes % 15 == 13);
    Expect.equals(19, dt.seconds);
    Expect.equals(999, dt.milliseconds);
  }

  static void testEquivalentYears() {
    // All hardcoded values come from V8. This means that the values are not
    // necessarily correct (see limitations of Date object in
    // EcmaScript 15.9.1 and in particular 15.9.1.8/9).
    Date dt = new Date.fromEpoch(-31485600000, const TimeZone.utc());
    Expect.equals(1969, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-63108000000, const TimeZone.utc());
    Expect.equals(1968, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-94644000000, const TimeZone.utc());
    Expect.equals(1967, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-126180000000, const TimeZone.utc());
    Expect.equals(1966, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-157716000000, const TimeZone.utc());
    Expect.equals(1965, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-2177402400000, const TimeZone.utc());
    Expect.equals(1901, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-5333076000000, const TimeZone.utc());
    Expect.equals(1801, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-8520285600000, const TimeZone.utc());
    Expect.equals(1700, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-14831719200000, const TimeZone.utc());
    Expect.equals(1500, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-59011408800000, const TimeZone.utc());
    Expect.equals(100, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(14, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-62011408800000, const TimeZone.utc());
    Expect.equals(4, dt.year);
    Expect.equals(12, dt.month);
    Expect.equals(8, dt.day);
    Expect.equals(8, dt.hours);
    Expect.equals(40, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    dt = new Date.fromEpoch(-64011408800000, const TimeZone.utc());
    Expect.equals(-59, dt.year);
    Expect.equals(7, dt.month);
    Expect.equals(24, dt.day);
    Expect.equals(5, dt.hours);
    Expect.equals(6, dt.minutes);
    Expect.equals(40, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    final int SECONDS_YEAR_2035 = 2051222400;
    dt = new Date.fromEpoch(SECONDS_YEAR_2035 * 1000 + 1, const TimeZone.utc());
    Expect.equals(2035, dt.year);
    Expect.equals(1, dt.month);
    Expect.equals(1, dt.day);
    Expect.equals(0, dt.hours);
    Expect.equals(0, dt.minutes);
    Expect.equals(0, dt.seconds);
    Expect.equals(1, dt.milliseconds);
    dt = new Date.fromEpoch(SECONDS_YEAR_2035 * 1000 - 1, const TimeZone.utc());
    Expect.equals(2034, dt.year);
    Expect.equals(12, dt.month);
    Expect.equals(31, dt.day);
    Expect.equals(23, dt.hours);
    Expect.equals(59, dt.minutes);
    Expect.equals(59, dt.seconds);
    Expect.equals(999, dt.milliseconds);
    dt = new Date.withTimeZone(2035, 1, 1, 0, 0, 0, 1, const TimeZone.utc());
    Expect.equals(SECONDS_YEAR_2035 * 1000 + 1, dt.value);
    dt = new Date.withTimeZone(2034, 12, 31, 23, 59, 59, 999,
                               const TimeZone.utc());
    Expect.equals(SECONDS_YEAR_2035 * 1000 - 1, dt.value);
    dt = new Date.fromEpoch(SECONDS_YEAR_2035 * 1000 + 1, new TimeZone.local());
    Expect.equals(true, (2035 == dt.year && 1 == dt.month && 1 == dt.day) ||
                        (2034 == dt.year && 12 == dt.month && 31 == dt.day));
    Expect.equals(0, dt.seconds);
    Expect.equals(1, dt.milliseconds);
    Date dt2 = new Date.withTimeZone(
        dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds,
        dt.milliseconds, new TimeZone.local());
    Expect.equals(dt.value, dt2.value);
    dt = new Date.fromEpoch(SECONDS_YEAR_2035 * 1000 - 1, new TimeZone.local());
    Expect.equals(true, (2035 == dt.year && 1 == dt.month && 1 == dt.day) ||
                        (2034 == dt.year && 12 == dt.month && 31 == dt.day));
    Expect.equals(59, dt.seconds);
    Expect.equals(999, dt.milliseconds);
    dt2 = new Date.withTimeZone(
        dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds,
        dt.milliseconds, new TimeZone.local());
    Expect.equals(dt.value, dt2.value);
  }

  static void testUTCGetters() {
    var dt = new Date.fromEpoch(1305140315000, const TimeZone.utc());
    Expect.equals(2011, dt.year);
    Expect.equals(5, dt.month);
    Expect.equals(11, dt.day);
    Expect.equals(18, dt.hours);
    Expect.equals(58, dt.minutes);
    Expect.equals(35, dt.seconds);
    Expect.equals(0, dt.milliseconds);
    Expect.equals(true, const TimeZone.utc() == dt.timeZone);
    Expect.equals(1305140315000, dt.value);
    dt = new Date.fromEpoch(-9999999, const TimeZone.utc());
    Expect.equals(1969, dt.year);
    Expect.equals(12, dt.month);
    Expect.equals(31, dt.day);
    Expect.equals(21, dt.hours);
    Expect.equals(13, dt.minutes);
    Expect.equals(20, dt.seconds);
    Expect.equals(1, dt.milliseconds);
  }

  static void testLocalGetters() {
    var dt1 = new Date.fromEpoch(1305140315000, new TimeZone.local());
    var dt2 = new Date.withTimeZone(dt1.year, dt1.month, dt1.day,
                                    dt1.hours, dt1.minutes, dt1.seconds,
                                    dt1.milliseconds,
                                    const TimeZone.utc());
    Duration zoneOffset = dt1.difference(dt2);
    Expect.equals(true, zoneOffset.inDays == 0);
    Expect.equals(true, zoneOffset.inHours.abs() <= 12);
    Expect.equals(dt1.year, dt2.year);
    Expect.equals(dt1.month, dt2.month);
    Expect.equals(true, (dt1.day - dt2.day).abs() <= 1);
    Expect.equals(true, dt1.hours < 24);
    // There are timezones with 0.5 or 0.25 hour offsets.
    Expect.equals(true,
                  (dt1.minutes == dt2.minutes) ||
                  ((dt1.minutes - dt2.minutes).abs() == 30) ||
                  ((dt1.minutes - dt2.minutes).abs() == 15));
    Expect.equals(dt1.seconds, dt2.seconds);
    Expect.equals(dt1.milliseconds, dt2.milliseconds);
  }

  static void testConstructors() {
    var dt1 = new Date.fromEpoch(1305140315000, new TimeZone.local());
    var dt3 = new Date(dt1.year, dt1.month, dt1.day, dt1.hours, dt1.minutes,
                       dt1.seconds, dt1.milliseconds);
    Expect.equals(dt1.value, dt3.value);
    Expect.equals(true, dt1 == dt3);
    dt3 = new Date.withTimeZone(
        dt1.year, dt1.month, dt1.day, dt1.hours, dt1.minutes,
        dt1.seconds, dt1.milliseconds, new TimeZone.local());
    Expect.equals(dt1.value, dt3.value);
    Expect.equals(true, dt1 == dt3);
    dt3 = new Date.withTimeZone(2011, 5, 11, 18, 58, 35, 0,
                                const TimeZone.utc());
    Expect.equals(dt1.value, dt3.value);
    Expect.equals(false, dt1 == dt3);
    var dt2 = dt1.changeTimeZone(new TimeZone.local());
    dt3 = new Date.withTimeZone(2011, 5, dt1.day,
                                dt1.hours, dt1.minutes, 35, 0,
                                new TimeZone.local());
    Expect.equals(dt2.value, dt3.value);
    Expect.equals(true, dt2 == dt3);
    dt1 = new Date.fromEpoch(-9999999, const TimeZone.utc());
    dt3 = new Date.withTimeZone(
        dt1.year, dt1.month, dt1.day, dt1.hours, dt1.minutes,
        dt1.seconds, dt1.milliseconds, const TimeZone.utc());
    Expect.equals(dt1.value, dt3.value);
  }

  static void testChangeTimeZone() {
    var dt1 = new Date.fromEpoch(1305140315000, new TimeZone.local());
    var dt2 = dt1.changeTimeZone(const TimeZone.utc());
    Expect.equals(dt1.value, dt2.value);
    var dt3 = new Date.fromEpoch(1305140315000, const TimeZone.utc());
    Expect.equals(dt1.value, dt3.value);
    Expect.equals(dt2.year, dt3.year);
    Expect.equals(dt2.month, dt3.month);
    Expect.equals(dt2.day, dt3.day);
    Expect.equals(dt2.hours, dt3.hours);
    Expect.equals(dt2.minutes, dt3.minutes);
    Expect.equals(dt2.seconds, dt3.seconds);
    Expect.equals(dt2.milliseconds, dt3.milliseconds);
    var dt4 = dt3.changeTimeZone(new TimeZone.local());
    Expect.equals(dt1.year, dt4.year);
    Expect.equals(dt1.month, dt4.month);
    Expect.equals(dt1.day, dt4.day);
    Expect.equals(dt1.hours, dt4.hours);
    Expect.equals(dt1.minutes, dt4.minutes);
    Expect.equals(dt1.seconds, dt4.seconds);
    Expect.equals(dt1.milliseconds, dt4.milliseconds);
  }

  static void testSubAdd() {
    var dt1 = new Date.fromEpoch(1305140315000, const TimeZone.utc());
    var dt2 = dt1.add(new Duration(milliseconds:
        3 * Duration.MILLISECONDS_PER_SECOND + 5));
    Expect.equals(dt1.year, dt2.year);
    Expect.equals(dt1.month, dt2.month);
    Expect.equals(dt1.day, dt2.day);
    Expect.equals(dt1.hours, dt2.hours);
    Expect.equals(dt1.minutes, dt2.minutes);
    Expect.equals(dt1.seconds + 3, dt2.seconds);
    Expect.equals(dt1.milliseconds + 5, dt2.milliseconds);
    var dt3 = dt2.subtract(new Duration(milliseconds:
        3 * Duration.MILLISECONDS_PER_SECOND + 5));
    Expect.equals(true, dt1 == dt3);
    Expect.equals(false, dt1 == dt2);
  }

  static void testDateStrings() {
    // TODO(floitsch): Clean up the Date API that deals with strings.
    var dt1 = new Date.fromString("2011-05-11 18:58:35Z");
    Expect.equals(1305140315000, dt1.value);
    var str = dt1.toString();
    var dt2 = new Date.fromString(str);
    Expect.equals(true, dt1 == dt2);
    var dt3 = dt1.changeTimeZone(const TimeZone.utc());
    str = dt3.toString();
    Expect.equals("2011-05-11 18:58:35.000Z", str);
  }

  static void testWeekday() {
    // 2011-10-06 is Summertime.
    var d = new Date(2011, 10, 6, 0, 45, 37, 0);
    Expect.equals(Date.THU, d.weekday);
    d = new Date.withTimeZone(2011, 10, 6, 0, 45, 37, 0, const TimeZone.utc());
    Expect.equals(Date.THU, d.weekday);
    d = new Date(2011, 10, 5, 23, 45, 37, 0);
    Expect.equals(Date.WED, d.weekday);
    d = new Date.withTimeZone(2011, 10, 5, 23, 45, 37, 0, const TimeZone.utc());
    Expect.equals(Date.WED, d.weekday);
    // 1970-01-01 is Wintertime.
    d = new Date(1970, 1, 1, 0, 0, 0, 1);
    Expect.equals(Date.THU, d.weekday);
    d = new Date.withTimeZone(1970, 1, 1, 0, 0, 0, 1, const TimeZone.utc());
    Expect.equals(Date.THU, d.weekday);
    d = new Date.withTimeZone(1969, 12, 31, 23, 59, 59, 999,
                              const TimeZone.utc());
    Expect.equals(Date.WED, d.weekday);
    d = new Date(1969, 12, 31, 23, 59, 59, 999);
    Expect.equals(Date.WED, d.weekday);
    d = new Date(2011, 10, 4, 23, 45, 37, 0);
    Expect.equals(Date.TUE, d.weekday);
    d = new Date(2011, 10, 3, 23, 45, 37, 0);
    Expect.equals(Date.MON, d.weekday);
    d = new Date(2011, 10, 2, 23, 45, 37, 0);
    Expect.equals(Date.SUN, d.weekday);
    d = new Date(2011, 10, 1, 23, 45, 37, 0);
    Expect.equals(Date.SAT, d.weekday);
    d = new Date(2011, 9, 30, 23, 45, 37, 0);
    Expect.equals(Date.FRI, d.weekday);
  }

  static void testMain() {
    testNow();
    testValue();
    testUTCGetters();
    testLocalGetters();
    testConstructors();
    testChangeTimeZone();
    testSubAdd();
    testDateStrings();
    testEquivalentYears();
    testFarAwayDates();
    testWeekday();
  }
}

main() {
  DateTest.testMain();
}
