/*
var grave = Sprite2D.create();
grave.setX(0);
grave.setY(0);
grave.show();

var frame = Asset.loadImageFrame2D("cursed_grave_texture.png",
  0, 0, 128, 128);
grave.render(frame);
*/

//var source = Asset.loadString("MMORPG.js");
//System.log('source:\n', source);

var animation = Animation2D.create();
for (var i = 0; i < 1024; ++i) {
  animation.addFrame(Asset.loadImageFrame2D("cursed_grave_texture.png",
    i, 0, 128, 128), 150);
  i += 128;
}

function update() {
  animation.update();
  //print('now: ' + System.now());
}
