from selenium.webdriver import Chrome, ChromeOptions
from time import sleep
import re
import sys
import os

# example: `python main.py LA001.mdl LA001.nbt`

base_path = os.path.dirname(os.path.abspath(__file__))

mdl = sys.argv[1]
nbt = sys.argv[2]
url = 'file://' + base_path + '/tracer.html'

if not os.path.isabs(mdl):
    mdl = os.path.normpath(os.path.join(base_path, mdl))
else:
    mdl = os.path.normpath(mdl)
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
model_input = driver.find_element_by_id('tgtModelFileIn')
model_input.send_keys(mdl)
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
# driver.save_screenshot('tracer.png')

driver.quit()  # ブラウザーを終了する
