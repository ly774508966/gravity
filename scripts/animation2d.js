System.register("Animation2D");

Animation2D.create = function() {
  var self = {};

  self.clear = function() {
    self.frameList = [];
    self.lastFrameIndex = -1;
    self.lastStopTime = 0;
    self.currentFrameIndex = 0;
    self.startTime = System.now();
  }

  self.addFrame = function(imageFrame, duration) {
    self.frameList.push([imageFrame, self.lastStopTime + duration]);
    self.lastStopTime += duration;
    return self;
  }

  self.setX = function(x) {
    self.sprite.setX(x);
    return self;
  }

  self.setY = function(y) {
    self.sprite.setY(y);
    return self;
  }

  self.setLayer = function(layer) {
    self.sprite.setLayer(layer);
    return self;
  }

  self.update = function() {
    if (0 == self.frameList.length) {
      return self;
    }
    var roundFinished = false;
    var elapsedTime = System.now() - self.startTime;
    if (elapsedTime > self.frameList[self.currentFrameIndex][1]) {
      if (self.currentFrameIndex + 1 >= self.frameList.length) {
        roundFinished = true;
      } else {
        self.currentFrameIndex += 1;
      }
    }
    if (self.lastFrameIndex != self.currentFrameIndex) {
      self.sprite.render(self.frameList[self.currentFrameIndex][0]);
      self.lastFrameIndex = self.currentFrameIndex;
    }
    if (roundFinished) {
      self.startTime = System.now();
      self.currentFrameIndex = 0;
    }
    return self;
  }

  /*
  self.dispose = function() {
    if (self.frameList && self.frameList.length) {
      for (var i = 0; i < self.frameList.length; ++i) {
        self.frameList[i][0].dispose();
      }
    }
  }*/

  self.sprite = Sprite2D.create();
  self.clear();

  return self;
};
