from selenium.webdriver import Chrome, ChromeOptions
from time import sleep
import re

options = ChromeOptions()
# ヘッドレスモードを有効にする（次の行をコメントアウトすると画面が表示される）。
options.add_argument('--headless')
# ChromeのWebDriverオブジェクトを作成する。
driver = Chrome(options=options)

driver.get('file:///home/garasubo/workspace/icfpc2018/downloads/html/tracer.html')

assert 'ICFP' in driver.title

# find input model file
model_input = driver.find_element_by_id('tgtModelFileIn')
model_input.send_keys("/home/garasubo/workspace/icfpc2018/downloads/problemsL/LA001_tgt.mdl")
tracer_input = driver.find_element_by_id('traceFileIn')
tracer_input.send_keys("/home/garasubo/workspace/icfpc2018/solutions/LA001.nbt")
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
driver.save_screenshot('tracer.png')



# 検索結果を表示する。

driver.quit()  # ブラウザーを終了する
