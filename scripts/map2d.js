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
    for (var col = 0; col < self.spriteCols; ++col) {
      for (var row = 0; row < self.spriteRows; ++row) {
        var tileIndex = row * self.tileWidth + col;
        var dispId = parseInt(self.tileData[tileIndex]);
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
        }
        index += 1;
      }
    }
    return self;
  }

  self.loadFromTiledMap = function(obj) {
    self.tileWidth = obj.tilewidth;
    self.tileHeight = obj.tileheight;
    self.worldWidth = obj.width * obj.tilewidth;
    self.worldHeight = obj.height * obj.tileheight;
    self.tileData = obj.layers[0].data;
    self.screenWidth = System.getWidth();
    self.screenHeight = System.getHeight();
    self.spriteCols = parseInt(self.screenWidth * 4 / self.tileWidth);
    self.spriteRows = parseInt(self.screenHeight * 8 / self.tileHeight);
    for (var col = 0; col < self.spriteCols; ++col) {
      for (var row = 0; row < self.spriteRows; ++row) {
        var animation = Animation2D.create();
        animation.setX((col - self.spriteCols / 4) * self.tileWidth +
          ((row % 2) ? self.tileWidth / 2 : 0));
        animation.setY((row - self.spriteRows / 4) * (self.tileHeight / 2));
        animation.setLayer(0);
        self.animations.push(animation);
        self.animationNames.push('');
      }
    }
    self.centerX = self.worldWidth / 2;
    self.centerY = self.worldHeight / 2;
    self.loadAnimations();
    return self;
  }

  self.setCenter = function(x, y) {
    if (self.centerX == x && self.centerY == y) {
      return self;
    }
    self.centerX = x;
    self.centerY = y;
    return self;
  }

  self.update = function() {
    var index = 0;
    for (var col = 0; col < self.spriteCols; ++col) {
      for (var row = 0; row < self.spriteRows; ++row) {
        self.animations[index].update();
        index += 1;
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
