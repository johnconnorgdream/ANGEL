def solution(l,ki):
    i = 0
    count = 0
    ret = []
    size = len(l)
    if(size < 3):
        return 0
    while(i < size):
        a = l[i]
        j = i + 1
        while(j < size):
            b = l[j]
            k = j+1
            while(k < size):
                c = l[k]
                if(a + b + c >= ki):
                    break
                else:
                    ret.add([a,b,c])
                    count = count + 1
                k = k + 1
            j = j + 1
        i = i + 1
    return ret
i = 0
l = [1,2,3,4,5]*2000
print(solution(l,3))