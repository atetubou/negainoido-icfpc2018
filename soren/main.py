from selenium.webdriver import Chrome, ChromeOptions
from time import sleep
import re
import sys
import os

# example: `python main.py LA001.mdl LA001.nbt`

base_path = os.path.dirname(os.path.abspath(__file__))

src = sys.argv[1]
tgt = sys.argv[2]
nbt = sys.argv[3]
url = 'file://' + base_path + '/tracer.html'

if src == '-':
    src = None
elif not os.path.isabs(src):
    src = os.path.normpath(os.path.join(base_path, src))
else:
    src = os.path.normpath(src)

if tgt == '-':
    tgt = None
elif not os.path.isabs(tgt):
    tgt = os.path.normpath(os.path.join(base_path, tgt))
else:
    tgt = os.path.normpath(tgt)
if not os.path.isabs(nbt):
    nbt = os.path.normpath(os.path.join(base_path, nbt))
else:
    nbt = os.path.normpath(nbt)

options = ChromeOptions()
# ヘッドレスモードを有効にする（次の行をコメントアウトすると画面が表示される）。
options.add_argument('--headless')
# ChromeのWebDriverオブジェクトを作成する。
driver = Chrome(options=options)

driver.get(url)
driver.save_screenshot('tracer.png')

assert 'ICFP' in driver.title

# find input model file
if src != None:
    src_input = driver.find_element_by_id('srcModelFileIn')
    src_input.send_keys(src)
else:
    driver.find_element_by_id('srcModelEmpty').click()

if tgt != None:
    model_input = driver.find_element_by_id('tgtModelFileIn')
    model_input.send_keys(tgt)
else:
    driver.find_element_by_id('tgtModelEmpty').click()

tracer_input = driver.find_element_by_id('traceFileIn')
tracer_input.send_keys(nbt)
driver.find_element_by_id('execTrace').click()

wait_limit = 30
stdout_text = ''
while wait_limit > 0:
    stdout_text = driver.find_element_by_id('stdout').text
    if stdout_text.find('Success') >= 0:
        break
    if stdout_text.find('Failure') >= 0:
        print('Found failure: ' + stdout_text)
        exit(1)
    sleep(1)
    wait_limit -= 1

energy = re.search('Energy:\s+([0-9]+)\n', stdout_text)
if energy == None:
    print('timeout')
    exit(1)

print(energy.group(1))

# スクリーンショットを撮る。
#driver.save_screenshot('tracer.png')

driver.quit()  # ブラウザーを終了する
