from lcov_cobertura import LcovCobertura

LCOV_FILE = 'build/coverage/test-coverage.info'
OUT_FILE = 'build/coverage/test-coverage.xml'

with open(LCOV_FILE) as fr:
    data = fr.read()

converter = LcovCobertura(data)
res = converter.convert()

with open(OUT_FILE, 'w') as fw:
    fw.write(res)
