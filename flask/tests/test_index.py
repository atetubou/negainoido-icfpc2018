import unittest
from urllib.request import Request, urlopen
from urllib.error import HTTPError

debug_server='http://localhost:8080'

class TestIndex(unittest.TestCase):
    def test_index(self):
        req = Request(debug_server + '/')
        try:
            res = urlopen(req)
            self.assertEqual(res.status, 200)
        except HTTPError as e:
            self.assertTrue(False)
