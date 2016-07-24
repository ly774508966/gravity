/*
var grave = Sprite2D.create();
grave.setX(0);
grave.setY(0);
grave.show();

var frame = Asset.loadImageFrame2D("cursed_grave_texture.png",
  0, 0, 128, 128);
grave.render(frame);
*/

System.setBackgroundColor(0xaaaaaa);

function loadImageFrame2DFromTextureInfo(textureName, info) {
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

function loadTextureInfoFromJson(jsonName) {
  var textureInfoMap = {};
  eval('var textureInfo = ' + Asset.loadString(jsonName));
  for (var i = 0; i < textureInfo.items.length; ++i) {
    var item = textureInfo.items[i];
    textureInfoMap[item.name] = item;
  }
  return textureInfoMap;
}

function loadObjectFromJson(jsonName) {
  var obj = {};
  eval('obj = ' + Asset.loadString(jsonName));
  return obj;
}

var textureInfoMap = loadTextureInfoFromJson("texture.json");

var runAnimation = Animation2D.create();
for (var i = 0; i < 7; ++i) {
  runAnimation.addFrame(loadImageFrame2DFromTextureInfo("texture.png",
    textureInfoMap["run_6_" + i + ".png"]), 150);
}
runAnimation.setX(128).setY(128);

var wyvernStillAnimation = Animation2D.create();
for (var i = 0; i < 4; ++i) {
  wyvernStillAnimation.addFrame(loadImageFrame2DFromTextureInfo("texture.png",
    textureInfoMap["wyvern_still_1_" + i + ".png"]), 200);
}
wyvernStillAnimation.setX(200).setY(128).setLayer(2);

var stillAnimation = Animation2D.create();
for (var i = 0; i < 4; ++i) {
  stillAnimation.addFrame(loadImageFrame2DFromTextureInfo("texture.png",
    textureInfoMap["still_0_" + i + ".png"]), 500);
}
stillAnimation.setX(0).setY(0);

var map = Map2D.create();
var tilemap = loadObjectFromJson('world.json');
map.loadFromTiledMap(tilemap);

/*
var animation = Animation2D.create();
for (var i = 0; i < 1024; ++i) {
  animation.addFrame(Asset.loadImageFrame2D("texture.png",
    i, 0, 128, 128), 150);
  i += 128;
}*/
//print(textureInfo);

function update() {
  map.update();
  runAnimation.update();
  wyvernStillAnimation.update();
  stillAnimation.update();
  //print('now: ' + System.now());
}
