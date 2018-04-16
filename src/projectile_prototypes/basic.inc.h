PP_BASIC(apple, 32, 32)
PP_BASIC(ball, 28, 28)
PP_BASIC(bigball, 40, 40)
PP_BASIC(bullet, 12, 20)
PP_BASIC(card, 17, 20)
PP_BASIC(crystal, 10, 20)
PP_BASIC(flea, 12, 12)
PP_BASIC(plainball, 25, 25)
PP_BASIC(rice, 10, 20)
PP_BASIC(soul, 80, 80)
PP_BASIC(thickrice, 10, 14)
PP_BASIC(wave, 25, 25)

// FIXME: these are player projectiles, and should use a different shader by default.
// It's not really appropriate to consider these "basic". Instead, we should add a
// different prototype for them, with slightly different defaults.
PP_BASIC(hghost, 22, 23)
PP_BASIC(marisa, 15, 50)
PP_BASIC(maristar, 25, 25)
PP_BASIC(youhoming, 50, 50)
PP_BASIC(youmu, 16, 24)

#undef PP_BASIC
