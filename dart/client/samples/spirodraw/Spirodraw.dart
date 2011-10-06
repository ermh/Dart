// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#library('spirodraw');

#import('dart:dom');
#source("ColorPicker.dart");
#resource("spirodraw.css");

void main() {
  new Spirodraw().go();
}

class Spirodraw {

  static double PI2 = Math.PI * 2;
  Document doc;
  // Scale factor used to scale wheel radius from 1-10 to pixels
  int RUnits, rUnits, dUnits;
  // Fixed radius, wheel radius, pen distance in pixels
  double R, r, d; 
  HTMLInputElement fixedRadiusSlider, wheelRadiusSlider, 
    penRadiusSlider, penWidthSlider, speedSlider;
  HTMLSelectElement inOrOut;
  HTMLLabelElement numTurns;
  HTMLDivElement mainDiv;
  num lastX, lastY;
  int height, width, xc, yc;
  int maxTurns;
  HTMLCanvasElement frontCanvas, backCanvas;
  CanvasRenderingContext2D front, back;
  HTMLCanvasElement paletteElement; 
  ColorPicker colorPicker;
  String penColor = "red";
  int penWidth;
  double rad = 0.0;
  double stepSize;
  bool animationEnabled = true;
  int numPoints;
  double speed;
  bool run;
  
  Spirodraw() {
    doc = window.document;
    inOrOut = doc.getElementById("in_out");
    fixedRadiusSlider = doc.getElementById("fixed_radius");
    wheelRadiusSlider = doc.getElementById("wheel_radius");
    penRadiusSlider = doc.getElementById("pen_radius");
    penWidthSlider = doc.getElementById("pen_width");
    speedSlider = doc.getElementById("speed");
    numTurns = doc.getElementById("num_turns");
    mainDiv = doc.getElementById("main");
    frontCanvas = doc.getElementById("canvas");
    front = frontCanvas.getContext("2d");
    backCanvas = doc.createElement("canvas");
    back = backCanvas.getContext("2d");
    paletteElement = doc.getElementById("palette");
    window.onresize = (EventListener) { onResize(); };
    initControlPanel();
  }

  void go() {
    onResize();
  }
  
  void onResize() {
    height = window.innerHeight;
    width = window.innerWidth - 270;
    yc = height~/2;
    xc = width~/2;
    frontCanvas.height = height;
    frontCanvas.width = width;
    backCanvas.height = height;
    backCanvas.width = width;
    clear();
  }
  
  void initControlPanel() {
    inOrOut.onchange = (EventListener) { refresh(); };
    fixedRadiusSlider.onchange = (EventListener) { refresh(); };
    wheelRadiusSlider.onchange = (EventListener) { refresh(); };
    speedSlider.onchange = (EventListener) { onSpeedChange(); };
    penRadiusSlider.onchange = (EventListener) { refresh(); };
    penWidthSlider.onchange = (EventListener) { onPenWidthChange(); };
    colorPicker = new ColorPicker(paletteElement);
    colorPicker.addListener((String color) { onColorChange(color); });
    doc.getElementById("start").onclick = (EventListener) { start(); };
    doc.getElementById("stop").onclick = (EventListener) { stop(); };
    doc.getElementById("clear").onclick = (EventListener) { clear(); };
    doc.getElementById("lucky").onclick = (EventListener) { lucky(); };
  }
  
  void onColorChange(String color) {
    penColor = color;
    drawFrame(rad);
  }

  void onSpeedChange() {
    speed = speedSlider.valueAsNumber;
    stepSize = calcStepSize();
  }
  
  void onPenWidthChange() {
    penWidth = penWidthSlider.valueAsNumber;
    drawFrame(rad);
  }
  
  void refresh() {
    stop();
    // Reset
    lastX = lastY = 0;
    // Compute fixed radius
    // based on starting diameter == min / 2, fixed radius == 10 units
    int min = Math.min(height, width);
    double pixelsPerUnit = min / 40;
    RUnits = fixedRadiusSlider.valueAsNumber;
    R = RUnits * pixelsPerUnit;
    // Scale inner radius and pen distance in units of fixed radius
    rUnits = wheelRadiusSlider.valueAsNumber;
    r = rUnits * R/RUnits * Math.parseInt(inOrOut.value);
    dUnits = penRadiusSlider.valueAsNumber;
    d = dUnits * R/RUnits;
    numPoints = calcNumPoints();
    maxTurns = calcTurns();
    onSpeedChange();
    numTurns.innerText = "0" + "/" + maxTurns;
    penWidth = penWidthSlider.valueAsNumber;
    drawFrame(0.0);
  }

  int calcNumPoints() {
    if ((dUnits==0) || (rUnits==0))
      // Empirically, treat it like an oval
      return 2;
    int gcf = gcf(RUnits, rUnits);
    int n = RUnits ~/ gcf;
    int d = rUnits ~/ gcf;
    if (n % 2 == 1)
      // odd
      return n;
    else if (d %2 == 1)
      return n;
    else
      return n~/2;
  }

  // TODO return optimum step size in radians
  double calcStepSize() => speed / 100 * maxTurns / numPoints;

  void drawFrame(double theta) {
    if (animationEnabled) {
      front.clearRect(0, 0, width, height);
      front.drawImage(backCanvas, 0, 0);
      drawFixed();
    }
    drawWheel(theta);
  }

  void animate(int time) {
    if (run && rad <= maxTurns * PI2) {
      rad+=stepSize;
      drawFrame(rad);
      num nTurns = rad / PI2;
      numTurns.innerText = nTurns.floor().toString() + "/" 
        + maxTurns.toString();
      window.webkitRequestAnimationFrame(animate, frontCanvas);
    } else {
      stop();
    }
  }
  
  void start() {
    refresh();
    rad = 0.0;
    run = true;
    window.webkitRequestAnimationFrame((int time){animate(time);},frontCanvas);
  }

  int calcTurns() {
    // compute ratio of wheel radius to big R then find LCM
    if ((dUnits==0) || (rUnits==0))
      return 1;
    int ru = rUnits.abs();
    int wrUnits = RUnits + rUnits;
    int g = gcf (wrUnits, ru);
    return ru ~/ g;
  }

  void stop() {
    run = false;
    // Show drawing only
    front.clearRect(0, 0, width, height);
    front.drawImage(backCanvas, 0, 0);
    // Reset angle
    rad = 0.0;
  }

  void clear() {
    stop();
    back.clearRect(0, 0, width, height);
    refresh();
  }

  /**
   * Choose random settings for wheel and pen, but
   * leave fixed radius alone as it often changes
   * things too much.
   */
  void lucky() {
    wheelRadiusSlider.valueAsNumber = Math.random() * 9;
    penRadiusSlider.valueAsNumber = Math.random() * 9;
    penWidthSlider.valueAsNumber = 1 + Math.random() * 9;
    colorPicker.setSelected(Math.random() * 215);
    start();
  }

  void drawFixed() {
    if (animationEnabled) {
      front.beginPath();
      front.setLineWidth(2);
      front.setStrokeStyle("gray");
      front.arc(xc, yc, R, 0, PI2, true);
      front.closePath();
      front.stroke();
    }
  }

  /**
   * Draw the wheel with its center at angle theta
   * with respect to the fixed wheel
   * 
   * @param theta
   */
  void drawWheel(double theta) {
    double wx = xc + ((R + r) * Math.cos(theta));
    double wy = yc - ((R + r) * Math.sin(theta));
    if (animationEnabled) {
      if (rUnits>0) {
        // Draw ring
        front.beginPath();
        front.arc(wx, wy, r.abs(), 0, PI2, true);
        front.closePath();
        front.stroke();
        // Draw center
        front.setLineWidth(1);
        front.beginPath();
        front.arc(wx, wy, 3, 0, PI2, true);
        front.setFillStyle("black");
        front.fill();
        front.closePath();
        front.stroke();
      }
    }
    drawTip(wx, wy, theta);
  }

  /**
   * Draw a rotating line that shows the wheel rolling and leaves
   * the pen trace
   * 
   * @param wx X coordinate of wheel center
   * @param wy Y coordinate of wheel center
   * @param theta Angle of wheel center with respect to fixed circle
   */
  void drawTip(double wx, double wy, double theta) {
    // Calc wheel rotation angle
    double rot = (r==0) ? theta : theta * (R+r) / r;
    // Find tip of line
    double tx = wx + d * Math.cos(rot);
    double ty = wy - d * Math.sin(rot);
    if (animationEnabled) {
      front.beginPath();
      front.setFillStyle(penColor);
      front.arc(tx, ty, penWidth/2+2, 0, PI2, true);
      front.fill();
      front.moveTo(wx, wy);
      front.setStrokeStyle("black");
      front.lineTo(tx, ty);
      front.closePath();
      front.stroke();
    }
    drawSegmentTo(tx, ty);
  }

  void drawSegmentTo(double tx, double ty) {
    if (lastX > 0) {
      back.beginPath();
      back.setStrokeStyle(penColor);
      back.setLineWidth(penWidth);
      back.moveTo(lastX, lastY);
      back.lineTo(tx, ty);
      back.closePath();
      back.stroke();
    }
    lastX = tx;
    lastY = ty;
  }
  
}

int gcf(int n, int d) {
  if (n==d)
    return n;
  int max = Math.max(n, d);
  for (int i = max ~/ 2; i > 1; i--)
    if ((n % i == 0) && (d % i == 0))
      return i;
  return 1;
}
