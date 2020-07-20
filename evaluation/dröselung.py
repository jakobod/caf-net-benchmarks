import csv

with open('data/send_timing_io.out', mode='r') as csv_file:
    csv_reader = csv.DictReader(csv_file)
    with open('data/send_timing_io_processed.out', mode='w') as out_file:
        csv_writer = csv.writer(out_file, delimiter=',')
        l1 = ['num']
        l2 = ['message-send-processing']
        for i, row in enumerate(csv_reader):
            l1.append(f'value{i}')
            l2.append(int(row[" socket"]) - int(row[" actor"]))
        csv_writer.writerow(l1)
        csv_writer.writerow(l2)


with open('data/send_timing_net.out', mode='r') as csv_file:
    csv_reader = csv.DictReader(csv_file)
    with open('data/send_timing_net_processed.out', mode='w') as out_file:
        csv_writer = csv.writer(out_file, delimiter=',')
        l1 = ['num']
        l2 = ['message-send-processing']
        for i, row in enumerate(csv_reader):
            l1.append(f'value{i}')
            l2.append(int(row[" socket"]) - int(row[" actor"]))
        csv_writer.writerow(l1)
        csv_writer.writerow(l2)


with open('data/recv_timing_io.out', mode='r') as csv_file:
    csv_reader = csv.DictReader(csv_file)
    with open('data/recv_timing_io_processed.out', mode='w') as out_file:
        csv_writer = csv.writer(out_file, delimiter=',')
        l1 = ['num']
        l2 = ['header-recv-processing']
        #l3 = ['payload-recv-processing']
        for i, row in enumerate(csv_reader):
            l1.append(f'value{i}')
            l2.append(int(row[" actor"]) - int(row["recv_header"]))
            #l3.append(int(row[" actor"]) - int(row[" recv_payload"]))
        csv_writer.writerow(l1)
        csv_writer.writerow(l2)
        #csv_writer.writerow(l3)


with open('data/recv_timing_net.out', mode='r') as csv_file:
    csv_reader = csv.DictReader(csv_file)
    with open('data/recv_timing_net_processed.out', mode='w') as out_file:
        csv_writer = csv.writer(out_file, delimiter=',')
        l1 = ['num']
        l2 = ['header-recv-processing']
        #l3 = ['payload-recv-processing']
        for i, row in enumerate(csv_reader):
            l1.append(f'value{i}')
            l2.append(int(row[" actor"]) - int(row[" recv_header"]))
            #l3.append(int(row[" actor"]) - int(row[" recv_payload"]))
        csv_writer.writerow(l1)
        csv_writer.writerow(l2)
        #csv_writer.writerow(l3)

with open('data/send_basp_timing.out', mode='r') as csv_file:
    csv_reader = csv.DictReader(csv_file)
    with open('data/send_basp_timing_processed.out', mode='w') as out_file:
        csv_writer = csv.writer(out_file, delimiter=',')
        n = ['num']
        d1 = ['send']
        d2 = ['enqueue']
        d3 = ['event']
        d4 = ['dequeue']
        d5 = ['application']
        d6 = ['write']
        for i, row in enumerate(csv_reader):
            n.append(f'value{i}')
            d1.append(int(row[" t2"]) - int(row["t1"]))
            d2.append(int(row[" t3"]) - int(row[" t2"]))
            d3.append(int(row[" t4"]) - int(row[" t3"]))
            d4.append(int(row[" t5"]) - int(row[" t4"]))
            d5.append(int(row[" t6"]) - int(row[" t5"]))
            d6.append(int(row[" t7"]) - int(row[" t6"]))
        csv_writer.writerow(n)
        csv_writer.writerow(d1)
        csv_writer.writerow(d2)
        csv_writer.writerow(d3)
        csv_writer.writerow(d4)
        csv_writer.writerow(d5)
        csv_writer.writerow(d6)


with open('data/recv_basp_timing.out', mode='r') as csv_file:
    csv_reader = csv.DictReader(csv_file)
    with open('data/recv_basp_timing_processed.out', mode='w') as out_file:
        csv_writer = csv.writer(out_file, delimiter=',')
        n = ['num']
        d1 = ['read header']
        d2 = ['process header']
        d3 = ['read payload']
        d4 = ['process payload']
        d5 = ['deliver']
        for i, row in enumerate(csv_reader):
            n.append(f'value{i}')
            d1.append(int(row[" t2"]) - int(row["t1"]))
            d2.append(int(row[" t3"]) - int(row[" t2"]))
            d3.append(int(row[" t4"]) - int(row[" t3"]))
            d4.append(int(row[" t5"]) - int(row[" t4"]))
            d5.append(int(row[" t6"]) - int(row[" t5"]))
        csv_writer.writerow(n)
        csv_writer.writerow(d1)
        csv_writer.writerow(d2)
        csv_writer.writerow(d3)
        csv_writer.writerow(d4)
        csv_writer.writerow(d5)


#with open('data/send_timing_net.out', mode='r') as csv_file:
#    csv_reader = csv.DictReader(csv_file)
#    with open('send_timing_net_processed.out', mode='w') as out_file:
#        csv_writer = csv.writer(out_file, delimiter=',')
#        csv_writer.writerow(['num', 'message-send-processing'])
#        for i, row in enumerate(csv_reader):
#            t1 = int(row[" socket"]) - int(row[" actor"])
#            csv_writer.writerow([i, t1])
#
#
#
#
#with open('data/recv_timing_io.out', mode='r') as csv_file:
#    csv_reader = csv.DictReader(csv_file)
#    with open('recv_timing_io_processed.out', mode='w') as out_file:
#        csv_writer = csv.writer(out_file, delimiter=',')
#        csv_writer.writerow(['num', 'header-recv-processing', 'payload-recv-processing'])
#        for i, row in enumerate(csv_reader):
#            t1 = int(row[" recv_payload"]) - int(row[" recv_header"])
#            t2 = int(row[" actor"]) - int(row[" recv_payload"])
#            csv_writer.writerow([i, t1, t2])
#
#
#with open('data/recv_timing_net.out', mode='r') as csv_file:
#    csv_reader = csv.DictReader(csv_file)
#    with open('recv_timing_net_processed.out', mode='w') as out_file:
#        csv_writer = csv.writer(out_file, delimiter=',')
#        csv_writer.writerow(['num', 'header-recv-processing', 'payload-recv-processing'])
#        for i, row in enumerate(csv_reader):
#            t1 = int(row[" recv_payload"]) - int(row[" recv_header"])
#            t2 = int(row[" actor"]) - int(row[" recv_payload"])
#            csv_writer.writerow([i, t1, t2])
