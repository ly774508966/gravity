System.register("Map2D");

Map2D.create = function() {
  var self = {};

  self.loadTextureInfoFromJson = function(jsonName) {
    var textureInfoMap = {};
    eval('var textureInfo = ' + Asset.loadString(jsonName));
    for (var i = 0; i < textureInfo.items.length; ++i) {
      var item = textureInfo.items[i];
      textureInfoMap[item.name] = item;
    }
    return textureInfoMap;
  }

  self.loadImageFrame2DFromTextureInfo = function(textureName, info) {
    var frame = Asset.loadImageFrame2D(textureName,
      info.left, info.top, info.width, info.height);
    if ('1' == info.rotated) {
      frame.flip();
    }
    if ('0' != info.trimOffsetLeft) {
      frame.setOffsetX(parseInt(info.trimOffsetLeft));
    }
    if ('0' != info.trimOffsetTop) {
      frame.setOffsetY(parseInt(info.trimOffsetTop));
    }
    return frame;
  }

  self.loadAnimations = function() {
    var startX = self.centerX - (self.screenWidth / 2);
    var startY = self.centerY - (self.screenHeight / 2);
    var startTileCol = startX / self.tileWidth;
    var startTileOffsetX = startX % self.tileWidth;
    var startTileRow = startY / self.tileHeight;
    var startTileOffsetY = startY % self.tileHeight;
    var index = 0;
    for (var row = 0; row < self.spriteRows && row < self.worldRows; ++row) {
      for (var col = 0; col < self.spriteCols && col < self.worldCols; ++col) {
        var tileIndex = row * self.worldCols + col;
        var dispId = parseInt(self.tileData[tileIndex]) - 1;
        if (dispId < 0) {
          index += 1;
          continue;
        }
        var tilesetRow = parseInt(dispId / 16);
        var tilesetCol = dispId % 16;
        var tilename = 'base_' + tilesetRow + '_' + tilesetCol + '.png';
        var animation = self.animations[index];
        if (self.animationNames[index] != tilename) {
          var frame = self.imageFrameMap[tilename];
          if (undefined == frame) {
            frame = self.loadImageFrame2DFromTextureInfo("texture.png",
              self.textureInfoMap[tilename]);
            self.imageFrameMap[tilename] = frame;
          }
          self.animationNames[index] = tilename;
          animation.clear();
          animation.addFrame(frame, 1000);
          animation.update();
        }
        index += 1;
      }
    }
    return self;
  }

  self.mapToScreen = function(x, y) {
    return [(x - y) * self.tileWidth / 2, (x + y) * self.tileHeight / 2];
  }

  self.screenToMap = function(x, y) {
    return [(x / (self.tileWidth / 2) + y / (self.tileHeight / 2)),
      (y / (self.tileHeight / 2) - x / (self.tileWidth / 2))];
  }

  self.updateTilePosition = function(col, row) {
    var screenPos = self.mapToScreen(self.origin[0] + col - self.centerX,
      self.origin[1] + row - self.centerY);
    var index = parseInt(row * self.spriteCols + col);
    var animation = self.animations[index];
    animation.setX(screenPos[0]);
    animation.setY(screenPos[1]);
    animation.update();
  }

  self.loadFromTiledMap = function(obj) {
    self.tileWidth = parseInt(obj.tilewidth);
    self.tileHeight = parseInt(obj.tileheight);
    self.worldCols = parseInt(obj.width);
    self.worldRows = parseInt(obj.height);
    self.worldWidth = self.worldCols * obj.tilewidth;
    self.worldHeight = self.worldRows * obj.tileheight;
    self.tileData = obj.layers[0].data;
    self.screenWidth = System.getWidth();
    self.screenHeight = System.getHeight();
    self.spriteCols = 25;//parseInt(self.screenWidth / self.tileWidth);
    self.spriteRows = 25;//parseInt(self.screenHeight / self.tileHeight);
    self.centerX = 0;
    self.centerY = 0;
    self.origin = self.screenToMap(self.screenWidth / 2 - self.tileWidth / 4,
      self.screenHeight / 2 - self.tileHeight / 4);
    System.log('center: [' + self.origin[0] + ', ' + self.origin[1] + ']');
    for (var row = 0; row < self.spriteRows; ++row) {
      for (var col = 0; col < self.spriteCols; ++col) {
        var screenPos = self.mapToScreen(self.origin[0] + col - self.centerX,
          self.origin[1] + row - self.centerY);
        var animation = Animation2D.create();
        animation.setX(screenPos[0]);
        animation.setY(screenPos[1]);
        animation.setLayer(0);
        self.animations.push(animation);
        self.animationNames.push('');
        //self.updateTilePosition(col, row);
      }
    }
    self.loadAnimations();
    return self;
  }

  self.setCenter = function(x, y) {
    if (self.centerX == x && self.centerY == y) {
      return self;
    }
    self.centerX = x;
    self.centerY = y;
    self.loadAnimations();
    return self;
  }

  self.update = function() {
    if (0 == self.animations.length) {
      return self;
    }
    self.centerX += 0.05;
    self.centerY += 0.05;
    for (var row = 0; row < self.spriteRows; ++row) {
      for (var col = 0; col < self.spriteCols; ++col) {
        self.updateTilePosition(col, row);
      }
    }
    return self;
  }

  self.centerX = 0;
  self.centerY = 0;
  self.tileWidth = 0;
  self.tileHeight = 0;
  self.worldWidth = 0;
  self.worlHeight = 0;
  self.worldCols = 0;
  self.worldRows = 0;
  self.animationCols = 0;
  self.animationRows = 0;
  self.animations = [];
  self.animationNames = [];
  self.tileData = null;
  self.screenWidth = 0;
  self.screenHeight = 0;
  self.imageFrameMap = {};
  self.animationMap = {};
  self.textureInfoMap = self.loadTextureInfoFromJson("texture.json");

  return self;
}
