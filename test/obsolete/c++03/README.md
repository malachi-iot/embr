This is an imperfect solution.

Reaches out to `../unity` folder

A better solution would be to multi target different c++ versions
and somehow expose both a lib and executable from `../unity` folder
itself - one for external linkages against `esp-idf` assuming it's possible
and the other for self contained testing