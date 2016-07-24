```sh
$ ../../../squeezer/src/squeezerw images --width 2048 --height 2048 --outputTexture texture.png --outputInfo texture.json --infoHeader "{\"textureWidth\":\"%W\", \"textureHeight\":\"%H\", \"items\":[\n" --infoFooter "]}" --infoBody "{\"name\":\"%n\", \"width\":\"%w\", \"height\":\"%h\", \"left\":\"%x\", \"top\":\"%y\", \"rotated\":\"%f\", \"trimOffsetLeft\":\"%l\", \"trimOffsetTop\":\"%t\", \"originWidth\":\"%c\", \"originHeight\":\"%r\"}" --infoSplit "\n,"
```

## References
1. http://clintbellanger.net/articles/isometric_math/  
2. http://archive.gamedev.net/archive/reference/programming/features/arttilebase/page2.html  
