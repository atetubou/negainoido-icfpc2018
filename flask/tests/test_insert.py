import unittest
import requests
import json
import random
import string

def nonce():
    return ''.join(random.choice(string.ascii_lowercase) for _ in range(10))

debug_server='http://localhost:8080'

class TestInsert(unittest.TestCase):
    def test_get(self):
        res = requests.get(debug_server + '/test_insert')
        self.assertEqual(res.status_code, 200)
    
    def test_get_json(self):
        res = requests.get(debug_server + '/test_insert/json')
        data = json.loads(res.text)
        self.assertEqual(data['result'], 'success')
        self.assertEqual(data['value'], None)
    
    def test_post(self):
        message = 'test message:' + nonce()
        data = { 'message': message }
        res = requests.post(debug_server + '/test_insert', data)
        self.assertEqual(res.status_code, 200)

    def test_post_invalid(self):
        res = requests.post(debug_server + '/test_insert', {})
        self.assertEqual(res.status_code, 400)
    
    def test_post_empty(self):
        res = requests.post(debug_server + '/test_insert', { 'message': ' '})
        self.assertEqual(res.status_code, 400)

    def test_post_json(self):
        message = 'test message:' + nonce()
        data = { 'message': message }
        res = requests.post(debug_server + '/test_insert/json', data)
        self.assertEqual(res.status_code, 200)

    def test_post_json_invalid(self):
        res = requests.post(debug_server + '/test_insert/json', {})
        data = json.loads(res.text)
        self.assertEqual(data['result'],'error')
    
    def test_post_json_empty(self):
        res = requests.post(debug_server + '/test_insert/json', { 'message': ' '})
        data = json.loads(res.text)
        self.assertEqual(data['result'],'error')
