var grave = Sprite2D.create();
grave.setX(0);
grave.setY(0);
grave.show();

var frame = Asset.loadImageFrame2D("cursed_grave_texture.png",
  0, 0, 128, 128);
grave.render(frame);

function update() {
  print('now: ' + System.now());
}
