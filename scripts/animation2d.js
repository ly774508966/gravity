System.register("Animation2D");

Animation2D.create = function() {
  var _this = {};

  _this.lastStopTime = 0;
  _this.startTime = System.now();
  _this.currentFrameIndex = 0;
  _this.frameList = [];
  _this.lastFrameIndex = -1;
  _this.sprite = Sprite2D.create();

  _this.addFrame = function(imageFrame, duration) {
    _this.frameList.push([imageFrame, _this.lastStopTime + duration]);
    _this.lastStopTime += duration;
    return _this;
  }

  _this.setX = function(x) {
    _this.sprite.setX(x);
    return _this;
  }

  _this.setY = function(y) {
    _this.sprite.setY(y);
    return _this;
  }

  _this.update = function() {
    if (0 == _this.frameList.length) {
      return _this;
    }
    var roundFinished = false;
    var elapsedTime = System.now() - _this.startTime;
    if (elapsedTime > _this.frameList[_this.currentFrameIndex][1]) {
      if (_this.currentFrameIndex + 1 >= _this.frameList.length) {
        roundFinished = true;
      } else {
        _this.currentFrameIndex += 1;
      }
    }
    if (_this.lastFrameIndex != _this.currentFrameIndex) {
      _this.sprite.render(_this.frameList[_this.currentFrameIndex][0]);
      _this.lastFrameIndex = _this.currentFrameIndex;
    }
    if (roundFinished) {
      _this.startTime = System.now();
      _this.currentFrameIndex = 0;
    }
    return _this;
  }

  return _this;
};
